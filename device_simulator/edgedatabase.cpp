#include "edgedatabase.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

namespace { QString utcNow(){return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);} }

EdgeDatabase::~EdgeDatabase(){if(m_db.isOpen())m_db.close();}

bool EdgeDatabase::open(const QString &path,const QStringList &chargerCodes,QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    m_db=QSqlDatabase::addDatabase("QSQLITE","charger-edge");m_db.setDatabaseName(path);
    if(!m_db.open()){if(error)*error=m_db.lastError().text();return false;}
    QSqlQuery q(m_db);q.exec("PRAGMA foreign_keys=ON");q.exec("PRAGMA journal_mode=WAL");
    const QStringList sql={
        "CREATE TABLE IF NOT EXISTS chargers(id INTEGER PRIMARY KEY AUTOINCREMENT,station_id INTEGER,code TEXT NOT NULL UNIQUE,type TEXT NOT NULL DEFAULT 'FAST',rated_power REAL NOT NULL DEFAULT 120,status TEXT NOT NULL DEFAULT 'IDLE',last_seen TEXT,total_sessions INTEGER NOT NULL DEFAULT 0,total_duration INTEGER NOT NULL DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS charge_orders(id INTEGER PRIMARY KEY AUTOINCREMENT,central_order_id INTEGER NOT NULL UNIQUE,user_id INTEGER NOT NULL,charger_id INTEGER NOT NULL REFERENCES chargers(id),status TEXT NOT NULL,mode TEXT NOT NULL,target REAL NOT NULL,start_at TEXT NOT NULL,end_at TEXT,energy REAL NOT NULL DEFAULT 0,duration INTEGER NOT NULL DEFAULT 0,amount REAL NOT NULL DEFAULT 0,base_price REAL NOT NULL DEFAULT 1.2)",
        "CREATE TABLE IF NOT EXISTS telemetry(id INTEGER PRIMARY KEY AUTOINCREMENT,charger_id INTEGER NOT NULL REFERENCES chargers(id),order_id INTEGER REFERENCES charge_orders(id),sampled_at TEXT NOT NULL,voltage REAL,current REAL,power REAL,soc REAL,energy_total REAL)",
        "CREATE INDEX IF NOT EXISTS idx_edge_orders_status ON charge_orders(status)",
        "CREATE INDEX IF NOT EXISTS idx_edge_telemetry_time ON telemetry(charger_id,sampled_at)"};
    for(const QString &statement:sql)if(!q.exec(statement)){if(error)*error=q.lastError().text();return false;}
    QSqlQuery add(m_db);add.prepare("INSERT OR IGNORE INTO chargers(code,last_seen) VALUES(?,?)");
    for(const QString &code:chargerCodes){add.bindValue(0,code);add.bindValue(1,utcNow());if(!add.exec()){if(error)*error=add.lastError().text();return false;}}
    return true;
}

QJsonArray EdgeDatabase::chargers(QString *error)
{
    QSqlQuery q(m_db);if(!q.exec("SELECT code,type,rated_power,status FROM chargers ORDER BY id")){if(error)*error=q.lastError().text();return{};}
    QJsonArray result;while(q.next())result.append(QJsonObject{{"code",q.value(0).toString()},{"type",q.value(1).toString()},{"ratedPower",q.value(2).toDouble()},{"status",q.value(3).toString()}});return result;
}

QJsonArray EdgeDatabase::pendingOrders(QString *error)
{
    QSqlQuery q(m_db);if(!q.exec("SELECT o.central_order_id,c.code,o.status,o.mode,o.target,o.energy,o.duration,o.amount,o.start_at,COALESCE(o.end_at,'') FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.status IN ('CHARGING','SYNC_PENDING') ORDER BY o.id")){if(error)*error=q.lastError().text();return{};}
    QJsonArray result;while(q.next())result.append(QJsonObject{{"orderId",q.value(0).toLongLong()},{"chargerCode",q.value(1).toString()},{"status",q.value(2).toString()},{"mode",q.value(3).toString()},{"target",q.value(4).toDouble()},{"energy",q.value(5).toDouble()},{"duration",q.value(6).toInt()},{"amount",q.value(7).toDouble()},{"startAt",q.value(8).toString()},{"endAt",q.value(9).toString()}});return result;
}

QJsonObject EdgeDatabase::startOrder(const QJsonObject &command,QString *error)
{
    const qint64 orderId=command.value("orderId").toVariant().toLongLong();const QString code=command.value("chargerCode").toString();
    QSqlQuery existing(m_db);existing.prepare("SELECT status FROM charge_orders WHERE central_order_id=?");existing.addBindValue(orderId);if(existing.exec()&&existing.next())return orderObject(orderId,error);
    if(!m_db.transaction()){if(error)*error=m_db.lastError().text();return{};}
    QSqlQuery charger(m_db);charger.prepare("SELECT id,status FROM chargers WHERE code=?");charger.addBindValue(code);
    if(!charger.exec()||!charger.next()||charger.value(1).toString()!="IDLE"){m_db.rollback();if(error)*error="本地充电桩不空闲";return{};}
    const qint64 chargerId=charger.value(0).toLongLong();
    QSqlQuery update(m_db);update.prepare("UPDATE chargers SET status='CHARGING',rated_power=?,last_seen=? WHERE id=?");update.addBindValue(command.value("ratedPower").toDouble(120));update.addBindValue(utcNow());update.addBindValue(chargerId);
    QSqlQuery insert(m_db);insert.prepare("INSERT INTO charge_orders(central_order_id,user_id,charger_id,status,mode,target,start_at,base_price) VALUES(?,?,?,'CHARGING',?,?,?,?)");insert.addBindValue(orderId);insert.addBindValue(command.value("userId").toVariant().toLongLong());insert.addBindValue(chargerId);insert.addBindValue(command.value("mode").toString());insert.addBindValue(command.value("target").toDouble());insert.addBindValue(utcNow());insert.addBindValue(command.value("basePrice").toDouble(1.2));
    if(!update.exec()||!insert.exec()||!m_db.commit()){m_db.rollback();if(error)*error=insert.lastError().text();return{};}return orderObject(orderId,error);
}

QJsonObject EdgeDatabase::orderObject(qint64 centralOrderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT o.central_order_id,c.code,o.status,o.energy,o.duration,o.amount,COALESCE(o.end_at,'') FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.central_order_id=?");q.addBindValue(centralOrderId);
    if(!q.exec()||!q.next()){if(error)*error="本地订单不存在";return{};}return{{"orderId",q.value(0).toLongLong()},{"chargerCode",q.value(1).toString()},{"status",q.value(2).toString()},{"energy",q.value(3).toDouble()},{"duration",q.value(4).toInt()},{"amount",q.value(5).toDouble()},{"endAt",q.value(6).toString()}};
}

QJsonObject EdgeDatabase::stopOrder(qint64 centralOrderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE charge_orders SET status='SYNC_PENDING',end_at=? WHERE central_order_id=? AND status='CHARGING'");q.addBindValue(utcNow());q.addBindValue(centralOrderId);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}
    if(q.numRowsAffected()==0){QJsonObject existing=orderObject(centralOrderId,error);if(existing.value("status").toString()!="SYNC_PENDING"&&error&&error->isEmpty())*error="订单已经停止";return existing;}
    QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE' WHERE id=(SELECT charger_id FROM charge_orders WHERE central_order_id=?)");release.addBindValue(centralOrderId);release.exec();return orderObject(centralOrderId,error);
}

bool EdgeDatabase::settleOrder(qint64 centralOrderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE charge_orders SET status='COMPLETED' WHERE central_order_id=? AND status IN ('SYNC_PENDING','COMPLETED')");q.addBindValue(centralOrderId);if(!q.exec()){if(error)*error=q.lastError().text();return false;}return q.numRowsAffected()==1;
}

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
    if(mode=="ENERGY"&&energy>target)energy=target;
    if(mode=="AMOUNT"&&price>0&&energy*price>target)energy=target/price;
    const double amount=energy*price;
    const bool finished=(mode=="AMOUNT"&&amount+0.000001>=target)||(mode=="ENERGY"&&energy+0.000001>=target)||(mode=="TIME"&&duration>=qRound64(target*60.0));
    if(!m_db.transaction()){if(error)*error=m_db.lastError().text();return{};}
    QSqlQuery update(m_db);update.prepare("UPDATE charge_orders SET energy=?,duration=?,amount=?,status=?,end_at=CASE WHEN ? THEN ? ELSE end_at END WHERE id=?");update.addBindValue(energy);update.addBindValue(duration);update.addBindValue(amount);update.addBindValue(finished?"SYNC_PENDING":"CHARGING");update.addBindValue(finished);update.addBindValue(utcNow());update.addBindValue(localId);
    QSqlQuery sample(m_db);sample.prepare("INSERT INTO telemetry(charger_id,order_id,sampled_at,voltage,current,power,soc,energy_total) VALUES(?,?,?,?,?,?,?,?)");sample.addBindValue(chargerId);sample.addBindValue(localId);sample.addBindValue(utcNow());sample.addBindValue(voltage);sample.addBindValue(current);sample.addBindValue(power);sample.addBindValue(soc);sample.addBindValue(energy);
    if(!update.exec()||!sample.exec()){m_db.rollback();if(error)*error=update.lastError().text();return{};}
    if(finished){QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE',total_sessions=total_sessions+1,total_duration=total_duration+? WHERE id=?");release.addBindValue(duration);release.addBindValue(chargerId);if(!release.exec()){m_db.rollback();if(error)*error=release.lastError().text();return{};}}
    if(!m_db.commit()){if(error)*error=m_db.lastError().text();return{};}return orderObject(centralId,error);
}

QString EdgeDatabase::chargerStatus(const QString &chargerCode,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT status FROM chargers WHERE code=?");q.addBindValue(chargerCode);if(!q.exec()||!q.next()){if(error)*error="本地充电桩不存在";return{};}return q.value(0).toString();
}

double EdgeDatabase::chargerRatedPower(const QString &chargerCode,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT rated_power FROM chargers WHERE code=?");q.addBindValue(chargerCode);if(!q.exec()||!q.next()){if(error)*error="本地充电桩不存在";return 0;}return q.value(0).toDouble();
}
