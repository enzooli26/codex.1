#include "edgedatabase.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

// 获取当前 UTC 时间 ISO 格式字符串
namespace { QString utcNow(){return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);} }

EdgeDatabase::EdgeDatabase(QObject *parent) : QObject(parent) {}
EdgeDatabase::~EdgeDatabase(){if(m_db.isOpen())m_db.close();}

// 打开数据库，创建表结构，按充电桩编号前缀自动分组站点
bool EdgeDatabase::open(const QString &path,const QStringList &chargerCodes,QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    m_db=QSqlDatabase::addDatabase("QSQLITE","charger-edge");m_db.setDatabaseName(path);
    if(!m_db.open()){if(error)*error=m_db.lastError().text();return false;}
    QSqlQuery q(m_db);q.exec("PRAGMA foreign_keys=ON");q.exec("PRAGMA journal_mode=WAL");
    const QStringList sql={
        "CREATE TABLE IF NOT EXISTS stations(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL,status TEXT NOT NULL DEFAULT 'ONLINE')",
        "CREATE TABLE IF NOT EXISTS chargers(id INTEGER PRIMARY KEY AUTOINCREMENT,station_id INTEGER,code TEXT NOT NULL UNIQUE,type TEXT NOT NULL DEFAULT 'FAST',rated_power REAL NOT NULL DEFAULT 120,status TEXT NOT NULL DEFAULT 'IDLE',last_seen TEXT,total_sessions INTEGER NOT NULL DEFAULT 0,total_duration INTEGER NOT NULL DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS charge_orders(id INTEGER PRIMARY KEY AUTOINCREMENT,central_order_id INTEGER NOT NULL UNIQUE,user_id INTEGER NOT NULL,charger_id INTEGER NOT NULL REFERENCES chargers(id),status TEXT NOT NULL,mode TEXT NOT NULL,target REAL NOT NULL,start_at TEXT NOT NULL,end_at TEXT,energy REAL NOT NULL DEFAULT 0,duration INTEGER NOT NULL DEFAULT 0,amount REAL NOT NULL DEFAULT 0,base_price REAL NOT NULL DEFAULT 1.2,payment_status TEXT NOT NULL DEFAULT 'PENDING',server_order_id INTEGER,disconnected_at TEXT)",
        "CREATE TABLE IF NOT EXISTS telemetry(id INTEGER PRIMARY KEY AUTOINCREMENT,charger_id INTEGER NOT NULL REFERENCES chargers(id),order_id INTEGER REFERENCES charge_orders(id),sampled_at TEXT NOT NULL,voltage REAL,current REAL,power REAL,soc REAL,energy_total REAL)",
        "CREATE INDEX IF NOT EXISTS idx_edge_orders_status ON charge_orders(status)",
        "CREATE INDEX IF NOT EXISTS idx_edge_telemetry_time ON telemetry(charger_id,sampled_at)"};
    for(const QString &statement:sql)if(!q.exec(statement)){if(error)*error=q.lastError().text();return false;}
    
    // 清理重复站点数据
    QSqlQuery cleanStations(m_db);
    cleanStations.prepare("DELETE FROM stations WHERE id NOT IN (SELECT MIN(id) FROM stations GROUP BY name)");
    cleanStations.exec();
    
    QSqlQuery add(m_db);add.prepare("INSERT OR IGNORE INTO chargers(code,last_seen) VALUES(?,?)");
    QMap<QString,QString> stationMap; // prefix -> station name
    stationMap["DL-SW-"] = "软件园充电站";
    stationMap["DL-GX-"] = "高新园区充电站";
    // 按编号前缀匹配站点，无匹配归入"未分组站点"
    for(const QString &code:chargerCodes){
        QString stationName;
        for(auto it=stationMap.constBegin();it!=stationMap.constEnd();++it){
            if(code.startsWith(it.key())){stationName=it.value();break;}
        }
        if(stationName.isEmpty())stationName="未分组站点";
        
        // 先确保站点存在 - 严格检查
        QSqlQuery st(m_db);st.prepare("SELECT id FROM stations WHERE name=?");
        st.addBindValue(stationName);
        int stationId=0;
        if(st.exec() && st.next()) {
            stationId = st.value(0).toInt();
        } else {
            // 如果站点不存在，才插入
            QSqlQuery insertSt(m_db);insertSt.prepare("INSERT OR IGNORE INTO stations(name) VALUES(?)");
            insertSt.addBindValue(stationName);
            insertSt.exec();
            // 再获取ID
            QSqlQuery getId(m_db);getId.prepare("SELECT id FROM stations WHERE name=?");
            getId.addBindValue(stationName);
            if(getId.exec() && getId.next()) stationId = getId.value(0).toInt();
        }
        
        add.bindValue(0,code);add.bindValue(1,utcNow());
        if(!add.exec()){if(error)*error=add.lastError().text();return false;}
        QSqlQuery upd(m_db);upd.prepare("UPDATE chargers SET station_id=? WHERE code=? AND station_id IS NULL");
        upd.bindValue(0,stationId);upd.bindValue(1,code);upd.exec();
    }
    return true;
}

// 查询所有站点及充电桩统计（总数/充电中/空闲）
QJsonArray EdgeDatabase::stations(QString *error)
{
    QSqlQuery q(m_db);
    const char *sql = "SELECT s.id,s.name,s.status,"
                      "COUNT(c.id),SUM(CASE WHEN c.status='CHARGING' THEN 1 ELSE 0 END),"
                      "SUM(CASE WHEN c.status='IDLE' THEN 1 ELSE 0 END) "
                      "FROM stations s LEFT JOIN chargers c ON c.station_id=s.id "
                      "GROUP BY s.id ORDER BY s.id";
    if(!q.exec(sql)){if(error)*error=q.lastError().text();return{};}
    QJsonArray result;
    while(q.next())result.append(QJsonObject{
        {"id",q.value(0).toInt()},{"name",q.value(1).toString()},{"status",q.value(2).toString()},
        {"total",q.value(3).toInt()},{"charging",q.value(4).toInt()},{"idle",q.value(5).toInt()}});
    return result;
}

// 查询所有充电桩列表
QJsonArray EdgeDatabase::chargers(QString *error)
{
    QSqlQuery q(m_db);if(!q.exec("SELECT code,type,rated_power,status FROM chargers ORDER BY id"))
    {
        if(error)*error=q.lastError().text();return{};
    }
    QJsonArray result;while(q.next())result.append(QJsonObject{{"code",q.value(0).toString()},{"type",q.value(1).toString()},{"ratedPower",q.value(2).toDouble()},{"status",q.value(3).toString()}});return result;
}

// 查询指定站点下的充电桩列表
QJsonArray EdgeDatabase::chargersByStation(int stationId, QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT code,type,rated_power,status FROM chargers WHERE station_id=? ORDER BY id");
    q.addBindValue(stationId);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}
    QJsonArray result;while(q.next())result.append(QJsonObject{{"code",q.value(0).toString()},{"type",q.value(1).toString()},{"ratedPower",q.value(2).toDouble()},{"status",q.value(3).toString()}});
    return result;
}

// 查询指定充电桩的活跃订单（CHARGING 或 SYNC_PENDING）
QJsonObject EdgeDatabase::activeOrderForCharger(const QString &chargerCode, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT o.central_order_id,o.status,o.mode,o.target,o.energy,o.duration,o.amount,COALESCE(o.end_at,'') "
              "FROM charge_orders o JOIN chargers c ON c.id=o.charger_id "
              "WHERE c.code=? AND o.status IN ('CHARGING','SYNC_PENDING') ORDER BY o.id DESC LIMIT 1");
    q.addBindValue(chargerCode);
    if(!q.exec()||!q.next()){if(error&&error->isEmpty())*error="";return{};}
    return{{"orderId",q.value(0).toLongLong()},{"status",q.value(1).toString()},{"mode",q.value(2).toString()},
        {"target",q.value(3).toDouble()},{"energy",q.value(4).toDouble()},{"duration",q.value(5).toInt()},
        {"amount",q.value(6).toDouble()},{"endAt",q.value(7).toString()}};
}

// 更新订单支付状态（PENDING/PAID）
bool EdgeDatabase::updateOrderPaymentStatus(qint64 orderId, const QString &status, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE charge_orders SET payment_status=? WHERE id=?");
    q.addBindValue(status);
    q.addBindValue(orderId);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    return q.numRowsAffected() == 1;
}

// 记录订单断网时间戳
bool EdgeDatabase::updateOrderDisconnectedAt(qint64 orderId, const QString &timestamp, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE charge_orders SET disconnected_at=? WHERE id=?");
    q.addBindValue(timestamp);
    q.addBindValue(orderId);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    return q.numRowsAffected() == 1;
}

// 查询待付款订单列表
QJsonArray EdgeDatabase::pendingPaymentOrders(QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT o.id,o.central_order_id,o.user_id,o.charger_id,o.status,o.mode,o.target,o.energy,o.duration,o.amount,o.start_at,o.end_at,c.code "
              "FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.payment_status='PENDING' ORDER BY o.id ASC");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}
    QJsonArray result;
    while(q.next()){
        result.append(QJsonObject{
            {"id",q.value(0).toLongLong()},{"centralOrderId",q.value(1).toLongLong()},{"userId",q.value(2).toLongLong()},
            {"chargerId",q.value(3).toLongLong()},{"status",q.value(4).toString()},{"mode",q.value(5).toString()},
            {"target",q.value(6).toDouble()},{"energy",q.value(7).toDouble()},{"duration",q.value(8).toInt()},
            {"amount",q.value(9).toDouble()},{"startAt",q.value(10).toString()},{"endAt",q.value(11).toString()},
            {"chargerCode",q.value(12).toString()}
        });
    }
    return result;
}

// 查询所有待处理订单（CHARGING 或 SYNC_PENDING）
QJsonArray EdgeDatabase::pendingOrders(QString *error)
{
    QSqlQuery q(m_db);if(!q.exec("SELECT o.central_order_id,c.code,o.status,o.mode,o.target,o.energy,o.duration,o.amount,o.start_at,COALESCE(o.end_at,'') FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.status IN ('CHARGING','SYNC_PENDING') ORDER BY o.id")){if(error)*error=q.lastError().text();return{};}
    QJsonArray result;while(q.next())result.append(QJsonObject{{"orderId",q.value(0).toLongLong()},{"chargerCode",q.value(1).toString()},{"status",q.value(2).toString()},{"mode",q.value(3).toString()},{"target",q.value(4).toDouble()},{"energy",q.value(5).toDouble()},{"duration",q.value(6).toInt()},{"amount",q.value(7).toDouble()},{"startAt",q.value(8).toString()},{"endAt",q.value(9).toString()}});return result;
}

// 创建充电订单：校验充电桩空闲、事务内插入订单并更新状态
QJsonObject EdgeDatabase::startOrder(const QJsonObject &command,QString *error)
{
    // 幂等：orderId 已存在则返回已有订单
    const qint64 orderId=command.value("orderId").toVariant().toLongLong();const QString code=command.value("chargerCode").toString();
    QSqlQuery existing(m_db);existing.prepare("SELECT status FROM charge_orders WHERE central_order_id=?");existing.addBindValue(orderId);if(existing.exec()&&existing.next())return orderObject(orderId,error);
    if(!m_db.transaction()){if(error)*error=m_db.lastError().text();return{};}
    QSqlQuery charger(m_db);charger.prepare("SELECT id,status FROM chargers WHERE code=?");charger.addBindValue(code);
    if(!charger.exec()||!charger.next()||charger.value(1).toString()!="IDLE"){m_db.rollback();if(error)*error="本地充电桩不空闲";return{};}
    const qint64 chargerId=charger.value(0).toLongLong();
    QSqlQuery update(m_db);update.prepare("UPDATE chargers SET status='CHARGING',rated_power=?,last_seen=? WHERE id=?");update.addBindValue(command.value("ratedPower").toDouble(120));update.addBindValue(utcNow());update.addBindValue(chargerId);
    QSqlQuery insert(m_db);insert.prepare("INSERT INTO charge_orders(central_order_id,user_id,charger_id,status,mode,target,start_at,base_price,server_order_id) VALUES(?,?,?,'CHARGING',?,?,?,?,-1)");insert.addBindValue(orderId);insert.addBindValue(command.value("userId").toVariant().toLongLong());insert.addBindValue(chargerId);insert.addBindValue(command.value("mode").toString());insert.addBindValue(command.value("target").toDouble());insert.addBindValue(utcNow());insert.addBindValue(command.value("basePrice").toDouble(1.2));
    if(!update.exec()||!insert.exec()||!m_db.commit()){m_db.rollback();if(error)*error=insert.lastError().text();return{};}
    return orderObject(orderId,error);
}

// 映射本地 orderId 与服务端 orderId
bool EdgeDatabase::updateServerOrderId(qint64 localOrderId, qint64 serverOrderId, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE charge_orders SET server_order_id=? WHERE id=?");
    q.addBindValue(serverOrderId);
    q.addBindValue(localOrderId);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    return q.numRowsAffected() == 1;
}

// 根据 centralOrderId 查询订单信息
QJsonObject EdgeDatabase::orderObject(qint64 centralOrderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT o.central_order_id,c.code,o.status,o.energy,o.duration,o.amount,COALESCE(o.end_at,'') FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.central_order_id=?");q.addBindValue(centralOrderId);
    if(!q.exec()||!q.next()){if(error)*error="本地订单不存在";return{};}return{{"orderId",q.value(0).toLongLong()},{"chargerCode",q.value(1).toString()},{"status",q.value(2).toString()},{"energy",q.value(3).toDouble()},{"duration",q.value(4).toInt()},{"amount",q.value(5).toDouble()},{"endAt",q.value(6).toString()}};
}

// 停止订单：CHARGING→SYNC_PENDING，释放充电桩
QJsonObject EdgeDatabase::stopOrder(qint64 centralOrderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE charge_orders SET status='SYNC_PENDING',end_at=? WHERE central_order_id=? AND status='CHARGING'");q.addBindValue(utcNow());q.addBindValue(centralOrderId);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}
    if(q.numRowsAffected()==0){QJsonObject existing=orderObject(centralOrderId,error);if(existing.value("status").toString()!="SYNC_PENDING"&&error&&error->isEmpty())*error="订单已经停止";return existing;}
    QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE' WHERE id=(SELECT charger_id FROM charge_orders WHERE central_order_id=?)");release.addBindValue(centralOrderId);release.exec();return orderObject(centralOrderId,error);
}

// 结算订单：SYNC_PENDING/COMPLETED→COMPLETED
bool EdgeDatabase::settleOrder(qint64 centralOrderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE charge_orders SET status='COMPLETED' WHERE central_order_id=? AND status IN ('SYNC_PENDING','COMPLETED')");q.addBindValue(centralOrderId);if(!q.exec()){if(error)*error=q.lastError().text();return false;}return q.numRowsAffected()==1;
}

// 中止订单：CHARGING→CANCELLED，释放充电桩（事务）
bool EdgeDatabase::abortOrder(qint64 centralOrderId,QString *error)
{
    if(!m_db.transaction()){if(error)*error=m_db.lastError().text();return false;}
    QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE' WHERE id=(SELECT charger_id FROM charge_orders WHERE central_order_id=?)");release.addBindValue(centralOrderId);
    QSqlQuery order(m_db);order.prepare("UPDATE charge_orders SET status='CANCELLED',end_at=? WHERE central_order_id=? AND status='CHARGING'");order.addBindValue(utcNow());order.addBindValue(centralOrderId);
    if(!release.exec()||!order.exec()||!m_db.commit()){m_db.rollback();if(error)*error=order.lastError().text();return false;}return true;
}

QJsonObject EdgeDatabase::tick(const QString &chargerCode,double voltage,double current,double power,double soc,QString *error)
{
    QSqlQuery find(m_db);find.prepare("SELECT o.id,o.central_order_id,o.mode,o.target,o.energy,o.duration,o.base_price,c.id FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE c.code=? AND o.status='CHARGING'");find.addBindValue(chargerCode);
    if(!find.exec()||!find.next())return{};
    const qint64 localId=find.value(0).toLongLong(),centralId=find.value(1).toLongLong(),chargerId=find.value(7).toLongLong();const QString mode=find.value(2).toString();const double target=find.value(3).toDouble(),oldEnergy=find.value(4).toDouble(),price=find.value(6).toDouble();const int oldDuration=find.value(5).toInt();
    int sampleSeconds=2;if(mode=="TIME")sampleSeconds=qMax(0,qMin(2,static_cast<int>(qRound64(target*60.0))-oldDuration));
    int duration=oldDuration+sampleSeconds;double energy=oldEnergy+power*sampleSeconds/3600.0;
    // ENERGY 模式下能量不超过目标值
    if(mode=="ENERGY"&&energy>target)energy=target;
    // AMOUNT 模式下金额不超过目标值
    if(mode=="AMOUNT"&&price>0&&energy*price>target)energy=target/price;
    const double amount=energy*price;
    // 三种模式达标条件判断
    const bool finished=(mode=="AMOUNT"&&amount+0.000001>=target)||(mode=="ENERGY"&&energy+0.000001>=target)||(mode=="TIME"&&duration>=qRound64(target*60.0));
    if(!m_db.transaction()){if(error)*error=m_db.lastError().text();return{};}
    QSqlQuery update(m_db);update.prepare("UPDATE charge_orders SET energy=?,duration=?,amount=?,status=?,end_at=CASE WHEN ? THEN ? ELSE end_at END WHERE id=?");update.addBindValue(energy);update.addBindValue(duration);update.addBindValue(amount);update.addBindValue(finished?"SYNC_PENDING":"CHARGING");update.addBindValue(finished);update.addBindValue(utcNow());update.addBindValue(localId);
    QSqlQuery sample(m_db);sample.prepare("INSERT INTO telemetry(charger_id,order_id,sampled_at,voltage,current,power,soc,energy_total) VALUES(?,?,?,?,?,?,?,?)");sample.addBindValue(chargerId);sample.addBindValue(localId);sample.addBindValue(utcNow());sample.addBindValue(voltage);sample.addBindValue(current);sample.addBindValue(power);sample.addBindValue(soc);sample.addBindValue(energy);
    if(!update.exec()||!sample.exec()){m_db.rollback();if(error)*error=update.lastError().text();return{};}
    // 订单完成后释放充电桩，累加统计
    if(finished){QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE',total_sessions=total_sessions+1,total_duration=total_duration+? WHERE id=?");release.addBindValue(duration);release.addBindValue(chargerId);if(!release.exec()){m_db.rollback();if(error)*error=release.lastError().text();return{};}}
    if(!m_db.commit()){if(error)*error=m_db.lastError().text();return{};}return orderObject(centralId,error);
}

// 查询充电桩状态
QString EdgeDatabase::chargerStatus(const QString &chargerCode,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT status FROM chargers WHERE code=?");q.addBindValue(chargerCode);if(!q.exec()||!q.next()){if(error)*error="本地充电桩不存在";return{};}return q.value(0).toString();
}

// 查询充电桩额定功率
double EdgeDatabase::chargerRatedPower(const QString &chargerCode,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT rated_power FROM chargers WHERE code=?");q.addBindValue(chargerCode);if(!q.exec()||!q.next()){if(error)*error="本地充电桩不存在";return 0;}return q.value(0).toDouble();
}

// 从服务端同步新增充电桩（自动创建站点）
bool EdgeDatabase::addChargerFromServer(const QString &code,const QString &type,double ratedPower,const QString &stationName,QString *error)
{
    // 查找或创建站点
    qint64 stationId=-1;
    QSqlQuery findStation(m_db);findStation.prepare("SELECT id FROM stations WHERE name=?");findStation.addBindValue(stationName);
    if(findStation.exec()&&findStation.next()){stationId=findStation.value(0).toLongLong();}
    else{
        QSqlQuery createStation(m_db);createStation.prepare("INSERT OR IGNORE INTO stations(name) VALUES(?)");createStation.addBindValue(stationName);
        if(!createStation.exec()){if(error)*error=createStation.lastError().text();return false;}
        QSqlQuery getId(m_db);getId.prepare("SELECT id FROM stations WHERE name=?");getId.addBindValue(stationName);
        if(getId.exec()&&getId.next())stationId=getId.value(0).toLongLong();
    }
    // 插入充电桩（INSERT OR IGNORE 避免重复）
    QSqlQuery add(m_db);add.prepare("INSERT OR IGNORE INTO chargers(code,type,rated_power,station_id,status,last_seen) VALUES(?,?,?,?,?,?)");
    add.bindValue(0,code);add.bindValue(1,type.isEmpty()?"FAST":type);add.bindValue(2,ratedPower>0?ratedPower:120);add.bindValue(3,stationId);add.bindValue(4,"IDLE");add.bindValue(5,utcNow());
    if(!add.exec()){if(error)*error=add.lastError().text();return false;}
    return true;
}

// 事务内级联删除：遥测→订单→充电桩
bool EdgeDatabase::removeChargerByCode(const QString &code,QString *error)
{
    QSqlQuery findId(m_db);findId.prepare("SELECT id FROM chargers WHERE code=?");findId.addBindValue(code);
    if(!findId.exec()||!findId.next()){if(error&&error->isEmpty())*error="充电桩不存在";return false;}
    const qint64 chargerId=findId.value(0).toLongLong();
    if(!m_db.transaction()){if(error)*error=m_db.lastError().text();return false;}
    QSqlQuery delTel(m_db);delTel.prepare("DELETE FROM telemetry WHERE charger_id=?");delTel.addBindValue(chargerId);
    if(!delTel.exec()){m_db.rollback();if(error)*error=delTel.lastError().text();return false;}
    QSqlQuery delOrd(m_db);delOrd.prepare("DELETE FROM charge_orders WHERE charger_id=?");delOrd.addBindValue(chargerId);
    if(!delOrd.exec()){m_db.rollback();if(error)*error=delOrd.lastError().text();return false;}
    QSqlQuery delCh(m_db);delCh.prepare("DELETE FROM chargers WHERE id=?");delCh.addBindValue(chargerId);
    if(!delCh.exec()){m_db.rollback();if(error)*error=delCh.lastError().text();return false;}
    if(!m_db.commit()){m_db.rollback();if(error)*error=m_db.lastError().text();return false;}
    return true;
}

// 从服务端同步更新充电桩属性
bool EdgeDatabase::updateChargerFromServer(const QString &code,const QString &type,double ratedPower,const QString &stationName,QString *error)
{
    qint64 stationId=-1;
    QSqlQuery findStation(m_db);findStation.prepare("SELECT id FROM stations WHERE name=?");findStation.addBindValue(stationName);
    if(findStation.exec()&&findStation.next()){stationId=findStation.value(0).toLongLong();}
    else{
        QSqlQuery createStation(m_db);createStation.prepare("INSERT OR IGNORE INTO stations(name) VALUES(?)");createStation.addBindValue(stationName);
        if(!createStation.exec()){if(error)*error=createStation.lastError().text();return false;}
        QSqlQuery getId(m_db);getId.prepare("SELECT id FROM stations WHERE name=?");getId.addBindValue(stationName);
        if(getId.exec()&&getId.next())stationId=getId.value(0).toLongLong();
    }
    QSqlQuery q(m_db);q.prepare("UPDATE chargers SET type=?,rated_power=?,station_id=? WHERE code=?");
    q.addBindValue(type.isEmpty()?"FAST":type);
    q.addBindValue(ratedPower>0?ratedPower:120);
    q.addBindValue(stationId);
    q.addBindValue(code);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    return q.numRowsAffected()==1;
}

// 差集清理：删除不在给定列表中的本地充电桩
QStringList EdgeDatabase::removeChargersNotIn(const QStringList &codes,QString *error)
{
    QStringList removed;
    QSqlQuery all(m_db);
    if(!all.exec("SELECT code FROM chargers")){if(error)*error=all.lastError().text();return removed;}
    QStringList localCodes;
    while(all.next())localCodes.append(all.value(0).toString());
    for(const QString &code:localCodes){
        if(!codes.contains(code)){
            QString e;
            if(removeChargerByCode(code, &e)) removed.append(code);
            else if(error&&error->isEmpty())*error=e;
        }
    }
    return removed;
}

// 清理无充电桩的空站点
QStringList EdgeDatabase::removeEmptyStations(QString *error)
{
    QStringList removed;
    QSqlQuery q(m_db);
    if(!q.exec("SELECT s.name FROM stations s WHERE (SELECT COUNT(*) FROM chargers c WHERE c.station_id=s.id)=0")){
        if(error)*error=q.lastError().text();return removed;
    }
    QStringList names;
    while(q.next())names.append(q.value(0).toString());
    for(const QString &name:names){
        QSqlQuery del(m_db);del.prepare("DELETE FROM stations WHERE name=?");del.addBindValue(name);
        if(del.exec())removed.append(name);
        else if(error&&error->isEmpty())*error=del.lastError().text();
    }
    return removed;
}

// 从服务端同步新增站点
bool EdgeDatabase::addStationFromServer(const QString &name,QString *error)
{
    if(name.isEmpty()){if(error)*error="站点名称不能为空";return false;}
    QSqlQuery q(m_db);q.prepare("INSERT OR IGNORE INTO stations(name) VALUES(?)");q.addBindValue(name);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    return true;
}

// 站点改名
bool EdgeDatabase::updateStationName(const QString &oldName,const QString &newName,QString *error)
{
    if(oldName.isEmpty()||newName.isEmpty()){if(error)*error="站点名称不能为空";return false;}
    QSqlQuery q(m_db);q.prepare("UPDATE stations SET name=? WHERE name=?");
    q.addBindValue(newName);q.addBindValue(oldName);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    return q.numRowsAffected()==1;
}

// 删除站点及其下所有充电桩
QStringList EdgeDatabase::removeStationByName(const QString &name,QString *error)
{
    QStringList removedChargers;
    if(name.isEmpty()){if(error)*error="站点名称不能为空";return removedChargers;}
    QSqlQuery findId(m_db);findId.prepare("SELECT id FROM stations WHERE name=?");findId.addBindValue(name);
    if(!findId.exec()||!findId.next()){if(error&&error->isEmpty())*error="站点不存在";return removedChargers;}
    const qint64 stationId=findId.value(0).toLongLong();
    QSqlQuery chargersQ(m_db);chargersQ.prepare("SELECT code FROM chargers WHERE station_id=?");chargersQ.addBindValue(stationId);
    if(chargersQ.exec()){
        while(chargersQ.next()){
            const QString code=chargersQ.value(0).toString();
            QString e;
            if(removeChargerByCode(code, &e)) removedChargers.append(code);
            else if(error&&error->isEmpty())*error=e;
        }
    }
    QSqlQuery delS(m_db);delS.prepare("DELETE FROM stations WHERE id=?");delS.addBindValue(stationId);
    if(!delS.exec()){if(error&&error->isEmpty())*error=delS.lastError().text();return removedChargers;}
    return removedChargers;
}
