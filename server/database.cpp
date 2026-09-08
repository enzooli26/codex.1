#include "database.h"
#include "passwordutils.h"
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

namespace {
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODate); }
QString legacyHash(const QString &password) { return QString::fromLatin1(QCryptographicHash::hash(password.toUtf8(),QCryptographicHash::Sha256).toHex()); }
}

Database::~Database()
{
    if (m_db.isOpen()) m_db.close();
}

bool Database::open(const QString &path, QString *error)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "server-main");
    m_db.setDatabaseName(path);
    if (!m_db.open()) { if (error) *error = m_db.lastError().text(); return false; }
    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys=ON");
    pragma.exec("PRAGMA journal_mode=WAL");
    if (!executeScript(":/db/schema.sql", error)) return false;
    const struct Migration { const char *table; const char *column; const char *definition; } migrations[] = {
        {"users","password_salt","TEXT"},{"users","password_hash","TEXT"},
        {"users","failed_attempts","INTEGER NOT NULL DEFAULT 0"},{"users","locked_at","TEXT"},
        {"admins","password_salt","TEXT"},{"admins","failed_attempts","INTEGER NOT NULL DEFAULT 0"},
        {"admins","locked_at","TEXT"},{"alarms","acknowledged_at","TEXT"}
    };
    for (const auto &migration : migrations)
        if (!ensureColumn(migration.table,migration.column,migration.definition,error)) return false;
    if (!executeScript(":/db/seed.sql", error)) return false;
    return true;
}

bool Database::ensureColumn(const QString &table,const QString &column,const QString &definition,QString *error)
{
    QSqlQuery info(m_db);
    if(!info.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))){if(error)*error=info.lastError().text();return false;}
    while(info.next())if(info.value(1).toString()==column)return true;
    QSqlQuery alter(m_db);
    if(!alter.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table,column,definition))){if(error)*error=alter.lastError().text();return false;}
    return true;
}

bool Database::executeScript(const QString &resourcePath, QString *error)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QStringList statements = QString::fromUtf8(file.readAll()).split(';', Qt::SkipEmptyParts);
    for (const QString &raw : statements) {
        const QString sql = raw.trimmed();
        if (sql.isEmpty()) continue;
        QSqlQuery query(m_db);
        if (!query.exec(sql)) { if (error) *error = query.lastError().text() + " | " + sql.left(120); return false; }
    }
    return true;
}

bool Database::begin(QString *error)
{
    if (m_db.transaction()) return true;
    if (error) *error = m_db.lastError().text();
    return false;
}
bool Database::commit(QString *error)
{
    if (m_db.commit()) return true;
    if (error) *error = m_db.lastError().text();
    return false;
}
void Database::rollback() { m_db.rollback(); }

QJsonObject Database::registerUser(const QString &phone,const QString &password,QString *error)
{
    if(!PasswordUtils::validPhone(phone)){if(error)*error="手机号格式错误";return{};}
    if(!PasswordUtils::validPassword(password)){if(error)*error="密码长度须为 6～64 位";return{};}
    QSqlQuery existing(m_db);existing.prepare("SELECT password_hash FROM users WHERE phone=?");existing.addBindValue(phone);
    if(!existing.exec()){if(error)*error=existing.lastError().text();return{};}
    const QString salt=PasswordUtils::createSalt(),hash=PasswordUtils::hashPassword(password,salt);
    if(existing.next()){
        if(!existing.value(0).toString().isEmpty()){if(error)*error="该手机号已注册";return{};}
        QSqlQuery upgrade(m_db);upgrade.prepare("UPDATE users SET password_salt=?,password_hash=?,failed_attempts=0,status='NORMAL',locked_at=NULL WHERE phone=?");upgrade.addBindValue(salt);upgrade.addBindValue(hash);upgrade.addBindValue(phone);
        if(!upgrade.exec()){if(error)*error=upgrade.lastError().text();return{};}
    }else{
        QSqlQuery insert(m_db);insert.prepare("INSERT INTO users(phone,nickname,password_salt,password_hash,balance,status,failed_attempts,created_at) VALUES(?,?,?,?,0,'NORMAL',0,?)");insert.addBindValue(phone);insert.addBindValue("用户"+phone.right(4));insert.addBindValue(salt);insert.addBindValue(hash);insert.addBindValue(now());
        if(!insert.exec()){if(error)*error=insert.lastError().text();return{};}
    }
    QSqlQuery q(m_db);q.prepare("SELECT id,phone,nickname,balance,status FROM users WHERE phone=?");q.addBindValue(phone);q.exec();q.next();
    return{{"id",q.value(0).toLongLong()},{"phone",q.value(1).toString()},{"nickname",q.value(2).toString()},{"balance",q.value(3).toDouble()},{"status",q.value(4).toString()}};
}

QJsonObject Database::loginUser(const QString &phone,const QString &password,QString *error)
{
    if(!PasswordUtils::validPhone(phone)||password.isEmpty()){if(error)*error="手机号或密码格式错误";return{};}
    QSqlQuery q(m_db);
    q.prepare("SELECT id,phone,nickname,balance,status,password_salt,password_hash,failed_attempts FROM users WHERE phone=?");
    q.addBindValue(phone);
    if (!q.exec()) { if (error) *error=q.lastError().text(); return {}; }
    if(!q.next()||q.value(6).toString().isEmpty()){if(error)*error="账号不存在，请先注册";return{};}
    if(q.value(4).toString()=="LOCKED"){if(error)*error="账户已锁定，请联系管理员解锁";return{};}
    if(q.value(4).toString()!="NORMAL"){if(error)*error="账户不可用，请联系管理员";return{};}
    const qint64 id=q.value(0).toLongLong();
    if(!PasswordUtils::constantTimeEquals(PasswordUtils::hashPassword(password,q.value(5).toString()),q.value(6).toString())){
        const int attempts=q.value(7).toInt()+1;QSqlQuery fail(m_db);fail.prepare("UPDATE users SET failed_attempts=?,status=CASE WHEN ?>=5 THEN 'LOCKED' ELSE status END,locked_at=CASE WHEN ?>=5 THEN ? ELSE locked_at END WHERE id=?");fail.addBindValue(attempts);fail.addBindValue(attempts);fail.addBindValue(attempts);fail.addBindValue(now());fail.addBindValue(id);fail.exec();
        if(error)*error=attempts>=5?"密码错误 5 次，账户已锁定":"密码错误，还可尝试 "+QString::number(5-attempts)+" 次";return{};
    }
    QSqlQuery reset(m_db);reset.prepare("UPDATE users SET failed_attempts=0,locked_at=NULL WHERE id=?");reset.addBindValue(id);reset.exec();
    return {{"id", q.value(0).toLongLong()}, {"phone", q.value(1).toString()},
            {"nickname", q.value(2).toString()}, {"balance", q.value(3).toDouble()},
            {"status", q.value(4).toString()}};
}

QJsonObject Database::recharge(qint64 userId,double amount,const QString &password,QString *error)
{
    if (amount <= 0 || amount > 10000) {
        if (error) *error = "充值金额必须在 0 到 10000 元之间";
        return {};
    }
    QSqlQuery auth(m_db);auth.prepare("SELECT password_salt,password_hash,status,failed_attempts FROM users WHERE id=?");auth.addBindValue(userId);
    if(!auth.exec()||!auth.next()){if(error)*error="请先登录";return{};}
    if(auth.value(2).toString()=="LOCKED"){if(error)*error="账户已锁定，请联系管理员解锁";return{};}
    if(auth.value(2).toString()!="NORMAL"){if(error)*error="账户不可用";return{};}
    if(!PasswordUtils::constantTimeEquals(PasswordUtils::hashPassword(password,auth.value(0).toString()),auth.value(1).toString())){
        const int attempts=auth.value(3).toInt()+1;QSqlQuery fail(m_db);fail.prepare("UPDATE users SET failed_attempts=?,status=CASE WHEN ?>=5 THEN 'LOCKED' ELSE status END,locked_at=CASE WHEN ?>=5 THEN ? ELSE locked_at END WHERE id=?");fail.addBindValue(attempts);fail.addBindValue(attempts);fail.addBindValue(attempts);fail.addBindValue(now());fail.addBindValue(userId);fail.exec();
        if(error)*error=attempts>=5?"密码错误 5 次，账户已锁定":"充值密码错误，还可尝试 "+QString::number(5-attempts)+" 次";return{};
    }
    QSqlQuery reset(m_db);reset.prepare("UPDATE users SET failed_attempts=0,locked_at=NULL WHERE id=?");reset.addBindValue(userId);reset.exec();
    if (!begin(error)) return {};
    QSqlQuery update(m_db);
    update.prepare("UPDATE users SET balance=balance+? WHERE id=? AND status='NORMAL'");
    update.addBindValue(amount); update.addBindValue(userId);
    if (!update.exec() || update.numRowsAffected()!=1) {
        rollback(); if(error)*error="用户不存在或已冻结"; return {};
    }
    QSqlQuery balance(m_db);
    balance.prepare("SELECT balance FROM users WHERE id=?"); balance.addBindValue(userId);
    if(!balance.exec()||!balance.next()){rollback();if(error)*error=balance.lastError().text();return{};}
    const double value=balance.value(0).toDouble();
    QSqlQuery flow(m_db);
    flow.prepare("INSERT INTO wallet_transactions(user_id,type,amount,balance_after,created_at) VALUES(?,'RECHARGE',?,?,?)");
    flow.addBindValue(userId);flow.addBindValue(amount);flow.addBindValue(value);flow.addBindValue(now());
    if(!flow.exec()||!commit(error)){rollback();return{};}
    return{{"balance",value}};
}

QJsonArray Database::userOrders(qint64 userId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT o.id,s.name,c.code,o.status,o.mode,o.energy,o.duration,o.amount,o.start_at,COALESCE(o.end_at,'--') FROM charge_orders o JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id WHERE o.user_id=? ORDER BY o.id DESC LIMIT 50");q.addBindValue(userId);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"station",q.value(1).toString()},{"charger",q.value(2).toString()},{"status",q.value(3).toString()},{"mode",q.value(4).toString()},{"energy",q.value(5).toDouble()},{"duration",q.value(6).toInt()},{"amount",q.value(7).toDouble()},{"startAt",q.value(8).toString()},{"endAt",q.value(9).toString()}});return a;
}

QJsonObject Database::userInfo(qint64 userId,QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT id,nickname,balance,status FROM users WHERE id=?");
    q.addBindValue(userId);
    if(!q.exec()){
        if(error)*error=q.lastError().text();
        return{};
    }
    if(!q.next()){
        if(error)*error="用户不存在";
        return{};
    }
    return{{"id",q.value(0).toLongLong()},
           {"nickname",q.value(1).toString()},
           {"balance",q.value(2).toDouble()},
           {"status",q.value(3).toString()}};
}

bool Database::loginAdmin(const QString &username, const QString &password, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("SELECT id,password_hash,status,COALESCE(password_salt,''),failed_attempts FROM admins WHERE username=?"); q.addBindValue(username);
    if (!q.exec()) { if (error) *error=q.lastError().text(); return false; }
    if (!q.next()) { if (error) *error="账号或密码错误"; return false; }
    if(q.value(2).toString()=="LOCKED"){if(error)*error="管理员账户已锁定，需要在数据库中由系统负责人解锁";return false;}
    if(q.value(2).toString()!="NORMAL"){if(error)*error="管理员账户不可用";return false;}
    const qint64 id=q.value(0).toLongLong();const QString stored=q.value(1).toString(),salt=q.value(3).toString();
    bool matched=false;
    if(salt.isEmpty())matched=PasswordUtils::constantTimeEquals(legacyHash(password),stored);
    else matched=PasswordUtils::constantTimeEquals(PasswordUtils::hashPassword(password,salt),stored);
    if(!matched){
        const int attempts=q.value(4).toInt()+1;QSqlQuery fail(m_db);fail.prepare("UPDATE admins SET failed_attempts=?,status=CASE WHEN ?>=5 THEN 'LOCKED' ELSE status END,locked_at=CASE WHEN ?>=5 THEN ? ELSE locked_at END WHERE id=?");fail.addBindValue(attempts);fail.addBindValue(attempts);fail.addBindValue(attempts);fail.addBindValue(now());fail.addBindValue(id);fail.exec();
        if(error)*error=attempts>=5?"密码错误 5 次，管理员账户已锁定":"账号或密码错误，还可尝试 "+QString::number(5-attempts)+" 次";return false;
    }
    QSqlQuery reset(m_db);
    if(salt.isEmpty()){const QString newSalt=PasswordUtils::createSalt();reset.prepare("UPDATE admins SET password_salt=?,password_hash=?,failed_attempts=0,locked_at=NULL WHERE id=?");reset.addBindValue(newSalt);reset.addBindValue(PasswordUtils::hashPassword(password,newSalt));reset.addBindValue(id);}
    else{reset.prepare("UPDATE admins SET failed_attempts=0,locked_at=NULL WHERE id=?");reset.addBindValue(id);}reset.exec();
    return true;
}

bool Database::registerAdmin(const QString &username,const QString &password,QString *error)
{
    if(!QRegularExpression(QStringLiteral("^[A-Za-z][A-Za-z0-9_]{3,31}$")).match(username).hasMatch()){if(error)*error="管理员账号须为 4～32 位字母、数字或下划线，并以字母开头";return false;}
    if(!PasswordUtils::validPassword(password)){if(error)*error="管理员密码长度须为 6～64 位";return false;}
    const QString salt=PasswordUtils::createSalt();QSqlQuery q(m_db);q.prepare("INSERT INTO admins(username,password_salt,password_hash,role,status,failed_attempts) VALUES(?,?,?,'ADMIN','NORMAL',0)");q.addBindValue(username);q.addBindValue(salt);q.addBindValue(PasswordUtils::hashPassword(password,salt));
    if(!q.exec()){if(error)*error=q.lastError().nativeErrorCode()=="19"?"管理员账号已存在":q.lastError().text();return false;}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','ADD_ADMIN','ADMIN',?,'SUCCESS',?)");log.addBindValue(q.lastInsertId());log.addBindValue(now());log.exec();return true;
}

QJsonArray Database::stationList(QString *error)
{
    QSqlQuery q(m_db);
    const char *sql = "SELECT s.id,s.name,s.address,s.longitude,s.latitude,s.base_price,"
                      "c.id,c.code,c.type,c.rated_power,c.status "
                      "FROM stations s JOIN chargers c ON c.station_id=s.id "
                      "WHERE s.status='ONLINE' ORDER BY s.id,c.id";
    if (!q.exec(sql)) { if (error) *error=q.lastError().text(); return {}; }
    QJsonArray result;
    while(q.next()) result.append(QJsonObject{{"id",q.value(0).toLongLong()},{"name",q.value(1).toString()},
        {"address",q.value(2).toString()},{"longitude",q.value(3).toDouble()},
        {"latitude",q.value(4).toDouble()},{"price",q.value(5).toDouble()},
        {"chargerId",q.value(6).toLongLong()},{"chargerCode",q.value(7).toString()},
        {"chargerType",q.value(8).toString()},{"ratedPower",q.value(9).toDouble()},
        {"chargerStatus",q.value(10).toString()},{"idle",q.value(10).toString()=="IDLE"?1:0},{"total",1}});
    return result;
}

QJsonObject Database::userChargeLive(qint64 userId,QString *error)
{
    QSqlQuery q(m_db);q.prepare(
        "SELECT o.id,o.mode,o.target,o.energy,o.duration,o.amount,c.code,c.rated_power,s.base_price,"
        "COALESCE(t.voltage,0),COALESCE(t.current,0),COALESCE(t.power,0),COALESCE(t.soc,0),COALESCE(t.sampled_at,'--') "
        "FROM charge_orders o JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id "
        "LEFT JOIN telemetry t ON t.id=(SELECT id FROM telemetry WHERE charger_id=c.id ORDER BY sampled_at DESC LIMIT 1) "
        "WHERE o.user_id=? AND o.status='CHARGING' ORDER BY o.id DESC LIMIT 1");q.addBindValue(userId);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}if(!q.next())return{{"charging",false}};
    const QString mode=q.value(1).toString();const double target=q.value(2).toDouble(),energy=q.value(3).toDouble(),amount=q.value(5).toDouble();const int duration=q.value(4).toInt();
    const double ratedPower=q.value(7).toDouble(),price=q.value(8).toDouble(),livePower=q.value(11).toDouble();const double estimatePower=livePower>0.01?livePower:ratedPower;int remaining=0;
    if(mode=="TIME")remaining=qMax(0,qRound(target*60.0)-duration);
    else if(mode=="ENERGY"&&estimatePower>0)remaining=qMax(0,qRound((target-energy)/estimatePower*3600.0));
    else if(mode=="AMOUNT"&&price>0&&estimatePower>0)remaining=qMax(0,qRound((target-amount)/price/estimatePower*3600.0));
    return{{"charging",true},{"orderId",q.value(0).toLongLong()},{"mode",mode},{"target",target},{"energy",energy},{"duration",duration},{"amount",amount},{"charger",q.value(6).toString()},{"voltage",q.value(9).toDouble()},{"current",q.value(10).toDouble()},{"power",livePower},{"soc",q.value(12).toDouble()},{"sampledAt",q.value(13).toString()},{"remainingSeconds",remaining}};
}

QJsonObject Database::deviceCatalog(QString *error)
{
    QJsonArray stations,chargers;QSqlQuery s(m_db),c(m_db);
    if(!s.exec("SELECT id,name,status FROM stations ORDER BY id")){if(error)*error=s.lastError().text();return{};}
    while(s.next())stations.append(QJsonObject{{"id",s.value(0).toLongLong()},{"name",s.value(1).toString()},{"status",s.value(2).toString()}});
    if(!c.exec("SELECT id,station_id,code,type,rated_power,status FROM chargers ORDER BY id")){if(error)*error=c.lastError().text();return{};}
    while(c.next())chargers.append(QJsonObject{{"id",c.value(0).toLongLong()},{"stationId",c.value(1).toLongLong()},{"code",c.value(2).toString()},{"type",c.value(3).toString()},{"ratedPower",c.value(4).toDouble()},{"status",c.value(5).toString()}});
    return{{"stations",stations},{"chargers",chargers}};
}

QJsonObject Database::createReservation(qint64 userId, qint64 chargerId, QString *error)
{
    if (!begin(error)) return {};
    QSqlQuery check(m_db);
    check.prepare("SELECT status FROM users WHERE id=?"); check.addBindValue(userId);
    if (!check.exec() || !check.next() || check.value(0).toString()!="NORMAL") { rollback(); if(error)*error="用户不可预约"; return {}; }
    QSqlQuery busy(m_db);
    busy.prepare("SELECT 1 FROM charge_orders WHERE user_id=? AND status IN ('CHARGING','PENDING_PAYMENT')"); busy.addBindValue(userId);
    if (!busy.exec() || busy.next()) { rollback(); if(error)*error="存在未完成订单"; return {}; }
    QSqlQuery lock(m_db);
    lock.prepare("UPDATE chargers SET status='RESERVED' WHERE id=? AND status='IDLE'"); lock.addBindValue(chargerId);
    if (!lock.exec() || lock.numRowsAffected()!=1) { rollback(); if(error)*error="充电桩当前不可预约"; return {}; }
    const QDateTime expires=QDateTime::currentDateTimeUtc().addSecs(20*60);
    QSqlQuery insert(m_db);
    insert.prepare("INSERT INTO reservations(user_id,charger_id,status,expires_at,created_at) VALUES(?,?,'ACTIVE',?,?)");
    insert.addBindValue(userId); insert.addBindValue(chargerId); insert.addBindValue(expires.toString(Qt::ISODate)); insert.addBindValue(now());
    if (!insert.exec() || !commit(error)) { rollback(); if(error && error->isEmpty())*error=insert.lastError().text(); return {}; }
    return {{"reservationId",insert.lastInsertId().toLongLong()},{"expiresAt",expires.toString(Qt::ISODate)}};
}

bool Database::cancelReservation(qint64 userId, qint64 reservationId, const QString &reason, QString *error)
{
    if (!begin(error)) return false;
    QSqlQuery find(m_db); find.prepare("SELECT charger_id FROM reservations WHERE id=? AND user_id=? AND status='ACTIVE'");
    find.addBindValue(reservationId); find.addBindValue(userId);
    if(!find.exec()||!find.next()){rollback();if(error)*error="预约不存在或已结束";return false;}
    const qint64 chargerId=find.value(0).toLongLong();
    QSqlQuery q(m_db); q.prepare("UPDATE reservations SET status='CANCELLED',cancel_reason=? WHERE id=?"); q.addBindValue(reason);q.addBindValue(reservationId);
    if(!q.exec()){rollback();if(error)*error=q.lastError().text();return false;}
    QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE' WHERE id=? AND status='RESERVED'");release.addBindValue(chargerId);
    if(!release.exec()||!commit(error)){rollback();return false;} return true;
}

QJsonObject Database::startCharge(qint64 userId, qint64 chargerId, const QString &mode, double target, QString *error)
{
    const QJsonObject pending=createPendingCharge(userId,chargerId,mode,target,error);
    if(pending.isEmpty())return{};
    return activatePendingCharge(userId,pending.value("orderId").toVariant().toLongLong(),error);
}

QJsonObject Database::createPendingCharge(qint64 userId,qint64 chargerId,const QString &mode,double target,QString *error)
{
    if(target<=0||!QStringList({"AMOUNT","ENERGY","TIME"}).contains(mode)){if(error)*error="充电目标无效";return{};}
    if(!begin(error))return{};
    QSqlQuery quote(m_db);
    quote.prepare("SELECT u.balance,u.status,c.code,c.rated_power,c.status,s.base_price "
                  "FROM users u,chargers c JOIN stations s ON s.id=c.station_id "
                  "WHERE u.id=? AND c.id=?");
    quote.addBindValue(userId);quote.addBindValue(chargerId);
    if(!quote.exec()||!quote.next()){rollback();if(error)*error="用户或充电桩不存在";return{};}
    const double balance=quote.value(0).toDouble();
    const QString userStatus=quote.value(1).toString();
    const QString chargerCode=quote.value(2).toString();
    const double ratedPower=quote.value(3).toDouble();
    const QString chargerStatus=quote.value(4).toString();
    const double price=quote.value(5).toDouble();
    if(userStatus!="NORMAL"){rollback();if(error)*error="用户状态异常";return{};}
    if(!QStringList({"IDLE","RESERVED"}).contains(chargerStatus)){rollback();if(error)*error="充电桩不可用";return{};}
    double required=target;
    if(mode=="ENERGY")required=target*price;
    else if(mode=="TIME")required=(target/60.0)*ratedPower*price;
    required=qRound64(required*100.0)/100.0;
    if(balance+0.0001<required){rollback();if(error)*error=QString("余额不足，预计至少需要 ¥%1（当前 ¥%2）").arg(required,0,'f',2).arg(balance,0,'f',2);return{};}
    QSqlQuery busy(m_db);busy.prepare("SELECT 1 FROM charge_orders WHERE user_id=? AND status IN ('STARTING','CHARGING','PENDING_PAYMENT')");busy.addBindValue(userId);
    if(!busy.exec()||busy.next()){rollback();if(error)*error="存在未完成订单";return{};}
    QSqlQuery lock(m_db);lock.prepare("UPDATE chargers SET status='STARTING' WHERE id=? AND status IN ('IDLE','RESERVED')");lock.addBindValue(chargerId);
    if(!lock.exec()||lock.numRowsAffected()!=1){rollback();if(error)*error="充电桩不可用";return{};}
    QSqlQuery insert(m_db);insert.prepare("INSERT INTO charge_orders(user_id,charger_id,status,mode,target,start_at,energy,duration,amount) VALUES(?,?,'STARTING',?,?,?,0,0,0)");
    insert.addBindValue(userId);insert.addBindValue(chargerId);insert.addBindValue(mode);insert.addBindValue(target);insert.addBindValue(now());
    if(!insert.exec()||!commit(error)){rollback();if(error&&error->isEmpty())*error=insert.lastError().text();return{};}
    return{{"orderId",insert.lastInsertId().toLongLong()},{"chargerId",chargerId},{"chargerCode",chargerCode},{"mode",mode},{"target",target},{"basePrice",price},{"ratedPower",ratedPower},{"requiredAmount",required}};
}

QJsonObject Database::activatePendingCharge(qint64 userId,qint64 orderId,QString *error)
{
    if(!begin(error))return{};
    QSqlQuery find(m_db);find.prepare("SELECT charger_id FROM charge_orders WHERE id=? AND user_id=? AND status='STARTING'");find.addBindValue(orderId);find.addBindValue(userId);
    if(!find.exec()||!find.next()){rollback();if(error)*error="待启动订单不存在";return{};}
    const qint64 chargerId=find.value(0).toLongLong();
    QSqlQuery order(m_db);order.prepare("UPDATE charge_orders SET status='CHARGING' WHERE id=?");order.addBindValue(orderId);
    QSqlQuery charger(m_db);charger.prepare("UPDATE chargers SET status='CHARGING' WHERE id=? AND status IN ('STARTING','IDLE','RESERVED','OFFLINE')");charger.addBindValue(chargerId);
    if(!order.exec()||!charger.exec()||charger.numRowsAffected()!=1){rollback();if(error)*error="订单激活失败";return{};}
    QSqlQuery reservation(m_db);reservation.prepare("UPDATE reservations SET status='USED' WHERE user_id=? AND charger_id=? AND status='ACTIVE'");reservation.addBindValue(userId);reservation.addBindValue(chargerId);reservation.exec();
    if(!commit(error)){rollback();return{};}
    return{{"orderId",orderId},{"status","CHARGING"}};
}

bool Database::cancelPendingCharge(qint64 orderId,const QString &reason,QString *error)
{
    if(!begin(error))return false;
    QSqlQuery find(m_db);find.prepare("SELECT charger_id,user_id FROM charge_orders WHERE id=? AND status='STARTING'");find.addBindValue(orderId);
    if(!find.exec()||!find.next()){rollback();return true;}
    const qint64 chargerId=find.value(0).toLongLong(),userId=find.value(1).toLongLong();
    QSqlQuery order(m_db);order.prepare("UPDATE charge_orders SET status='CANCELLED',end_at=? WHERE id=?");order.addBindValue(now());order.addBindValue(orderId);
    QSqlQuery charger(m_db);charger.prepare("UPDATE chargers SET status=CASE WHEN EXISTS(SELECT 1 FROM reservations WHERE user_id=? AND charger_id=? AND status='ACTIVE') THEN 'RESERVED' ELSE 'IDLE' END WHERE id=? AND status='STARTING'");charger.addBindValue(userId);charger.addBindValue(chargerId);charger.addBindValue(chargerId);
    if(!order.exec()||!charger.exec()){rollback();if(error)*error=order.lastError().text();return false;}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('SYSTEM','CANCEL_DEVICE_START','ORDER',?,?,?)");log.addBindValue(orderId);log.addBindValue(reason);log.addBindValue(now());log.exec();
    if(!commit(error)){rollback();return false;}return true;
}

QJsonObject Database::activeOrderForStop(qint64 userId,qint64 orderId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT o.id,o.charger_id,c.code FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.id=? AND o.user_id=? AND o.status='CHARGING'");q.addBindValue(orderId);q.addBindValue(userId);
    if(!q.exec()||!q.next()){if(error)*error="活动订单不存在";return{};}
    return{{"orderId",q.value(0).toLongLong()},{"chargerId",q.value(1).toLongLong()},{"chargerCode",q.value(2).toString()}};
}

QJsonObject Database::completeChargeFromDevice(qint64 orderId,double energy,int duration,const QString &endAt,QString *error)
{
    if(energy<0||duration<0){if(error)*error="充电桩上报的结算数据无效";return{};}
    if(!begin(error))return{};
    QSqlQuery find(m_db);find.prepare("SELECT o.user_id,o.charger_id,o.status,s.base_price FROM charge_orders o JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id WHERE o.id=?");find.addBindValue(orderId);
    if(!find.exec()||!find.next()){rollback();if(error)*error="中心订单不存在";return{};}
    const qint64 userId=find.value(0).toLongLong(),chargerId=find.value(1).toLongLong();
    const QString oldStatus=find.value(2).toString();
    if(oldStatus=="COMPLETED"){QSqlQuery done(m_db);done.prepare("SELECT amount,energy,duration FROM charge_orders WHERE id=?");done.addBindValue(orderId);done.exec();done.next();rollback();return{{"orderId",orderId},{"userId",userId},{"amount",done.value(0).toDouble()},{"energy",done.value(1).toDouble()},{"duration",done.value(2).toInt()},{"status","COMPLETED"}};}
    if(oldStatus!="CHARGING"&&oldStatus!="STARTING"){rollback();if(error)*error="订单状态不可结算";return{};}
    const double amount=qRound64(energy*find.value(3).toDouble()*100.0)/100.0;
    QSqlQuery wallet(m_db);wallet.prepare("UPDATE users SET balance=balance-? WHERE id=? AND balance>=?");wallet.addBindValue(amount);wallet.addBindValue(userId);wallet.addBindValue(amount);
    if(!wallet.exec()||wallet.numRowsAffected()!=1){rollback();if(error)*error="余额不足，订单需人工处理";return{};}
    QSqlQuery update(m_db);update.prepare("UPDATE charge_orders SET status='COMPLETED',end_at=?,energy=?,duration=?,amount=? WHERE id=?");update.addBindValue(endAt.isEmpty()?now():endAt);update.addBindValue(energy);update.addBindValue(duration);update.addBindValue(amount);update.addBindValue(orderId);
    if(!update.exec()){rollback();if(error)*error=update.lastError().text();return{};}
    QSqlQuery flow(m_db);flow.prepare("INSERT INTO wallet_transactions(user_id,type,amount,balance_after,related_order_id,created_at) SELECT id,'CHARGE',-?,balance,?,? FROM users WHERE id=?");flow.addBindValue(amount);flow.addBindValue(orderId);flow.addBindValue(now());flow.addBindValue(userId);
    QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE',total_sessions=total_sessions+1,total_duration=total_duration+? WHERE id=?");release.addBindValue(duration);release.addBindValue(chargerId);
    if(!flow.exec()||!release.exec()||!commit(error)){rollback();return{};}
    return{{"orderId",orderId},{"userId",userId},{"amount",amount},{"energy",energy},{"duration",duration},{"status","COMPLETED"}};
}

QJsonObject Database::syncDeviceOrder(qint64 orderId,const QString &chargerCode,const QString &status,double energy,int duration,const QString &endAt,QString *error)
{
    QSqlQuery owner(m_db);owner.prepare("SELECT o.id FROM charge_orders o JOIN chargers c ON c.id=o.charger_id WHERE o.id=? AND c.code=?");owner.addBindValue(orderId);owner.addBindValue(chargerCode);
    if(!owner.exec()||!owner.next()){if(error)*error="订单与充电桩不匹配";return{};}
    if(status=="SYNC_PENDING"||status=="COMPLETED")return completeChargeFromDevice(orderId,energy,duration,endAt,error);
    QSqlQuery q(m_db);q.prepare("UPDATE charge_orders SET status='CHARGING',energy=MAX(energy,?),duration=MAX(duration,?) WHERE id=? AND status IN ('STARTING','CHARGING')");q.addBindValue(energy);q.addBindValue(duration);q.addBindValue(orderId);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}
    updateHeartbeat(chargerCode,"CHARGING",error);
    return{{"orderId",orderId},{"status","CHARGING"}};
}

QJsonObject Database::stopCharge(qint64 userId, qint64 orderId, QString *error)
{
    if(!begin(error))return{};
    QSqlQuery find(m_db);find.prepare("SELECT charger_id,energy,start_at FROM charge_orders WHERE id=? AND user_id=? AND status='CHARGING'");find.addBindValue(orderId);find.addBindValue(userId);
    if(!find.exec()||!find.next()){rollback();if(error)*error="活动订单不存在";return{};}
    const qint64 chargerId=find.value(0).toLongLong(); const double energy=find.value(1).toDouble();
    const int elapsed=qMax(1,static_cast<int>(QDateTime::fromString(find.value(2).toString(),Qt::ISODate).secsTo(QDateTime::currentDateTimeUtc())));
    QSqlQuery price(m_db);price.prepare("SELECT s.base_price FROM chargers c JOIN stations s ON s.id=c.station_id WHERE c.id=?");price.addBindValue(chargerId);price.exec();price.next();
    const double amount=qRound64(energy*price.value(0).toDouble()*100.0)/100.0;
    QSqlQuery update(m_db);update.prepare("UPDATE charge_orders SET status='COMPLETED',end_at=?,amount=?,duration=MAX(duration,?) WHERE id=?");update.addBindValue(now());update.addBindValue(amount);update.addBindValue(elapsed);update.addBindValue(orderId);
    if(!update.exec()){rollback();if(error)*error=update.lastError().text();return{};}
    QSqlQuery wallet(m_db);wallet.prepare("UPDATE users SET balance=balance-? WHERE id=? AND balance>=?");wallet.addBindValue(amount);wallet.addBindValue(userId);wallet.addBindValue(amount);
    if(!wallet.exec()||wallet.numRowsAffected()!=1){rollback();if(error)*error="余额不足，订单转人工结算";return{};}
    QSqlQuery flow(m_db);flow.prepare("INSERT INTO wallet_transactions(user_id,type,amount,balance_after,related_order_id,created_at) SELECT id,'CHARGE',-?,balance,?,? FROM users WHERE id=?");flow.addBindValue(amount);flow.addBindValue(orderId);flow.addBindValue(now());flow.addBindValue(userId);if(!flow.exec()){rollback();if(error)*error=flow.lastError().text();return{};}
    QSqlQuery release(m_db);release.prepare("UPDATE chargers SET status='IDLE',total_sessions=total_sessions+1,total_duration=total_duration+? WHERE id=?");release.addBindValue(elapsed);release.addBindValue(chargerId);release.exec();
    if(!commit(error)){rollback();return{};} return {{"orderId",orderId},{"amount",amount},{"energy",energy},{"status","COMPLETED"}};
}

bool Database::updateHeartbeat(const QString &chargerCode, const QString &status, QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE chargers SET status=?,last_seen=? WHERE code=?");
    q.addBindValue(status);q.addBindValue(now());q.addBindValue(chargerCode);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}return q.numRowsAffected()==1;
}

bool Database::markDeviceOffline(const QStringList &chargerCodes,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE chargers SET status='OFFLINE' WHERE code=?");
    for(const QString &code:chargerCodes){q.bindValue(0,code);if(!q.exec()){if(error)*error=q.lastError().text();return false;}}
    return true;
}

bool Database::insertTelemetry(const QString &chargerCode,double voltage,double current,double power,double soc,QString *error)
{
    QSqlQuery find(m_db);find.prepare("SELECT id FROM chargers WHERE code=?");find.addBindValue(chargerCode);
    if(!find.exec()||!find.next()){if(error)*error="未知设备";return false;} const qint64 chargerId=find.value(0).toLongLong();
    QSqlQuery q(m_db);q.prepare("INSERT INTO telemetry(charger_id,sampled_at,voltage,current,power,soc) VALUES(?,?,?,?,?,?)");
    q.addBindValue(chargerId);q.addBindValue(now());q.addBindValue(voltage);q.addBindValue(current);q.addBindValue(power);q.addBindValue(soc);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}
    QSqlQuery order(m_db);
    order.prepare("UPDATE charge_orders SET energy=energy+(?/1800.0),duration=duration+2 WHERE charger_id=? AND status='CHARGING'");
    order.addBindValue(power);order.addBindValue(chargerId);
    if(!order.exec()){if(error)*error=order.lastError().text();return false;}
    return true;
}

QJsonObject Database::adminSummary(int trendDays, QString *error)
{
    trendDays=trendDays==30?30:7;
    QJsonObject result;
    QSqlQuery q(m_db);
    if(!q.exec("SELECT COUNT(*),SUM(CASE WHEN status='IDLE' THEN 1 ELSE 0 END),SUM(CASE WHEN status='CHARGING' THEN 1 ELSE 0 END),SUM(CASE WHEN status='FAULT' THEN 1 ELSE 0 END) FROM chargers")||!q.next()){if(error)*error=q.lastError().text();return{};}
    result["chargers"]=q.value(0).toInt();result["idle"]=q.value(1).toInt();result["charging"]=q.value(2).toInt();result["fault"]=q.value(3).toInt();result["trendDays"]=trendDays;
    QSqlQuery metrics(m_db);
    if(!metrics.exec("SELECT COALESCE(SUM(CASE WHEN date(end_at)=date('now','localtime') THEN amount END),0),"
                     "COALESCE(SUM(CASE WHEN strftime('%Y-%m',end_at)=strftime('%Y-%m','now','localtime') THEN amount END),0),"
                     "COALESCE(SUM(amount),0),SUM(CASE WHEN date(start_at)=date('now','localtime') THEN 1 ELSE 0 END) "
                     "FROM charge_orders WHERE status='COMPLETED'")||!metrics.next()){if(error)*error=metrics.lastError().text();return{};}
    result["todayRevenue"]=metrics.value(0).toDouble();result["monthRevenue"]=metrics.value(1).toDouble();
    result["revenue"]=metrics.value(2).toDouble();result["todayOrders"]=metrics.value(3).toInt();
    QSqlQuery users(m_db);if(users.exec("SELECT COUNT(*) FROM users")&&users.next())result["users"]=users.value(0).toInt();
    QJsonArray trend;
    QSqlQuery tq(m_db);const QString trendSql=QString("WITH RECURSIVE days(d) AS (SELECT date('now','localtime','-%1 day') UNION ALL SELECT date(d,'+1 day') FROM days WHERE d<date('now','localtime')) SELECT d,COALESCE(SUM(o.amount),0) FROM days LEFT JOIN charge_orders o ON date(o.end_at)=d AND o.status='COMPLETED' GROUP BY d ORDER BY d").arg(trendDays-1);if(!tq.exec(trendSql)){if(error)*error=tq.lastError().text();return{};}
    while(tq.next())trend.append(QJsonObject{{"date",tq.value(0).toString()},{"amount",tq.value(1).toDouble()}});result["revenueTrend"]=trend;
    QJsonArray ranking;
    QSqlQuery rq(m_db);rq.exec("SELECT s.name,COALESCE(SUM(o.amount),0) revenue FROM stations s LEFT JOIN chargers c ON c.station_id=s.id LEFT JOIN charge_orders o ON o.charger_id=c.id AND o.status='COMPLETED' GROUP BY s.id ORDER BY revenue DESC LIMIT 5");
    while(rq.next())ranking.append(QJsonObject{{"name",rq.value(0).toString()},{"revenue",rq.value(1).toDouble()}});result["stationRanking"]=ranking;
    QJsonArray active;
    QSqlQuery aq(m_db);aq.exec("SELECT o.id,u.phone,s.name,c.code,o.energy,o.duration FROM charge_orders o JOIN users u ON u.id=o.user_id JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id WHERE o.status='CHARGING' ORDER BY o.id DESC LIMIT 8");
    while(aq.next())active.append(QJsonObject{{"id",aq.value(0).toLongLong()},{"phone",aq.value(1).toString()},{"station",aq.value(2).toString()},{"charger",aq.value(3).toString()},{"energy",aq.value(4).toDouble()},{"duration",aq.value(5).toInt()}});result["activeOrders"]=active;
    QJsonArray faults;
    QSqlQuery fq(m_db);fq.exec("SELECT c.id,c.code,s.name,COALESCE(c.last_seen,'--') FROM chargers c JOIN stations s ON s.id=c.station_id WHERE c.status='FAULT' ORDER BY c.id DESC LIMIT 8");
    while(fq.next())faults.append(QJsonObject{{"id",fq.value(0).toLongLong()},{"code",fq.value(1).toString()},{"station",fq.value(2).toString()},{"lastSeen",fq.value(3).toString()}});result["faultChargers"]=faults;
    return result;
}

QJsonArray Database::adminStations(QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT s.id,s.name,s.address,s.longitude,s.latitude,s.base_price,s.status,COUNT(c.id),SUM(CASE WHEN c.status='IDLE' THEN 1 ELSE 0 END),SUM(CASE WHEN c.status='FAULT' THEN 1 ELSE 0 END) FROM stations s LEFT JOIN chargers c ON c.station_id=s.id GROUP BY s.id ORDER BY s.id DESC");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"name",q.value(1).toString()},{"address",q.value(2).toString()},{"longitude",q.value(3).toDouble()},{"latitude",q.value(4).toDouble()},{"price",q.value(5).toDouble()},{"status",q.value(6).toString()},{"total",q.value(7).toInt()},{"idle",q.value(8).toInt()},{"fault",q.value(9).toInt()}});return a;
}

QJsonArray Database::adminChargers(QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT c.id,c.code,s.name,c.type,c.rated_power,c.status,c.total_sessions,c.total_duration,COALESCE(c.last_seen,'--'),CASE WHEN c.status='FAULT' THEN '异常' WHEN c.status='OFFLINE' THEN '离线' WHEN c.last_seen IS NULL THEN '未上报' WHEN (julianday('now')-julianday(c.last_seen))*86400>120 THEN '心跳超时' ELSE '健康' END,CASE WHEN c.status='FAULT' THEN 20 WHEN c.status='OFFLINE' THEN 0 WHEN c.last_seen IS NULL THEN 60 WHEN (julianday('now')-julianday(c.last_seen))*86400>120 THEN 40 ELSE 100 END,COALESCE(t.voltage,0),COALESCE(t.current,0),COALESCE(t.power,0),COALESCE(t.soc,0) FROM chargers c JOIN stations s ON s.id=c.station_id LEFT JOIN telemetry t ON t.id=(SELECT id FROM telemetry WHERE charger_id=c.id ORDER BY sampled_at DESC LIMIT 1) ORDER BY c.id DESC");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"code",q.value(1).toString()},{"station",q.value(2).toString()},{"chargerType",q.value(3).toString()},{"power",q.value(4).toDouble()},{"status",q.value(5).toString()},{"sessions",q.value(6).toInt()},{"duration",q.value(7).toInt()},{"lastSeen",q.value(8).toString()},{"health",QString("%1 (%2)").arg(q.value(9).toString()).arg(q.value(10).toInt())},{"healthScore",q.value(10).toInt()},{"voltage",q.value(11).toDouble()},{"current",q.value(12).toDouble()},{"livePower",q.value(13).toDouble()},{"soc",q.value(14).toDouble()}});return a;
}

QJsonArray Database::adminOrders(const QString &statusFilter,const QString &keyword,QString *error)
{
    const QString status=statusFilter.trimmed().toUpper(),term="%"+keyword.trimmed()+"%";
    QSqlQuery q(m_db);q.prepare("SELECT o.id,u.phone,s.name,c.code,o.status,o.mode,o.target,o.energy,o.duration,o.amount,o.start_at,COALESCE(o.end_at,'--') FROM charge_orders o JOIN users u ON u.id=o.user_id JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id WHERE (?='' OR o.status=?) AND (?='%%' OR u.phone LIKE ? OR s.name LIKE ? OR c.code LIKE ? OR CAST(o.id AS TEXT) LIKE ?) ORDER BY o.id DESC LIMIT 300");
    q.addBindValue(status);q.addBindValue(status);q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"phone",q.value(1).toString()},{"station",q.value(2).toString()},{"charger",q.value(3).toString()},{"status",q.value(4).toString()},{"mode",q.value(5).toString()},{"target",q.value(6).toDouble()},{"energy",q.value(7).toDouble()},{"duration",q.value(8).toInt()},{"amount",q.value(9).toDouble()},{"startAt",q.value(10).toString()},{"endAt",q.value(11).toString()}});return a;
}

QJsonArray Database::adminUsers(const QString &phoneFilter,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT id,phone,nickname,balance,status,failed_attempts,COALESCE(locked_at,'--'),created_at FROM users WHERE phone LIKE ? ORDER BY id DESC LIMIT 300");q.addBindValue("%"+phoneFilter+"%");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"phone",q.value(1).toString()},{"nickname",q.value(2).toString()},{"balance",q.value(3).toDouble()},{"status",q.value(4).toString()},{"failedAttempts",q.value(5).toInt()},{"lockedAt",q.value(6).toString()},{"createdAt",q.value(7).toString()}});return a;
}

QJsonArray Database::adminLogs(const QString &keyword,QString *error)
{
    const QString term="%"+keyword.trimmed()+"%";QSqlQuery q(m_db);q.prepare("SELECT id,COALESCE(actor_type,'SYSTEM'),COALESCE(actor_id,0),action,COALESCE(target_type,'--'),COALESCE(target_id,0),COALESCE(result,'--'),created_at FROM operation_logs WHERE (?='%%' OR COALESCE(actor_type,'SYSTEM') LIKE ? OR action LIKE ? OR COALESCE(target_type,'--') LIKE ? OR COALESCE(result,'--') LIKE ? OR CAST(id AS TEXT) LIKE ?) ORDER BY id DESC LIMIT 300");q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);q.addBindValue(term);
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"actor",q.value(1).toString()+"#"+q.value(2).toString()},{"action",q.value(3).toString()},{"target",q.value(4).toString()+"#"+q.value(5).toString()},{"result",q.value(6).toString()},{"createdAt",q.value(7).toString()}});return a;
}

QJsonArray Database::adminAlarms(QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT a.id,a.level,a.type,c.code,s.name,a.message,a.status,a.created_at,COALESCE(a.acknowledged_at,'--'),COALESCE(a.resolved_at,'--'),COALESCE(m.status,'未派单') FROM alarms a LEFT JOIN chargers c ON c.id=a.charger_id LEFT JOIN stations s ON s.id=c.station_id LEFT JOIN maintenance_orders m ON m.alarm_id=a.id ORDER BY CASE a.status WHEN 'OPEN' THEN 0 WHEN 'ACKNOWLEDGED' THEN 1 ELSE 2 END,a.id DESC LIMIT 300");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray items;while(q.next())items.append(QJsonObject{{"id",q.value(0).toLongLong()},{"level",q.value(1).toString()},{"type",q.value(2).toString()},{"charger",q.value(3).toString()},{"station",q.value(4).toString()},{"message",q.value(5).toString()},{"status",q.value(6).toString()},{"createdAt",q.value(7).toString()},{"acknowledgedAt",q.value(8).toString()},{"resolvedAt",q.value(9).toString()},{"maintenance",q.value(10).toString()}});return items;
}

QJsonArray Database::adminMaintenance(QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT m.id,COALESCE(m.alarm_id,0),c.code,s.name,m.issue,m.assignee,COALESCE(m.scheduled_at,'--'),m.status,m.created_at,COALESCE(m.started_at,'--'),COALESCE(m.completed_at,'--'),COALESCE(m.result,'--') FROM maintenance_orders m JOIN chargers c ON c.id=m.charger_id JOIN stations s ON s.id=c.station_id ORDER BY CASE m.status WHEN 'IN_PROGRESS' THEN 0 WHEN 'DISPATCHED' THEN 1 ELSE 2 END,m.id DESC LIMIT 300");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray items;while(q.next())items.append(QJsonObject{{"id",q.value(0).toLongLong()},{"alarmId",q.value(1).toLongLong()},{"charger",q.value(2).toString()},{"station",q.value(3).toString()},{"issue",q.value(4).toString()},{"assignee",q.value(5).toString()},{"scheduledAt",q.value(6).toString()},{"status",q.value(7).toString()},{"createdAt",q.value(8).toString()},{"startedAt",q.value(9).toString()},{"completedAt",q.value(10).toString()},{"result",q.value(11).toString()}});return items;
}

int Database::inspectDevices(QString *error)
{
    if(!begin(error))return -1;int created=0;const QString timestamp=now();
    struct Rule{const char *level;const char *type;const char *message;const char *condition;};
    const Rule rules[]={
        {"CRITICAL","DEVICE_FAULT","设备主动上报故障，请立即检查","c.status='FAULT'"},
        {"CRITICAL","NETWORK_OFFLINE","设备与中心服务器连接断开","c.status='OFFLINE'"},
        {"WARNING","HEARTBEAT_TIMEOUT","设备超过 120 秒未上报心跳","c.last_seen IS NOT NULL AND c.status NOT IN ('OFFLINE','FAULT') AND (julianday('now')-julianday(c.last_seen))*86400>120"}
    };
    for(const Rule &rule:rules){
        QSqlQuery insert(m_db);insert.prepare(QString("INSERT INTO alarms(charger_id,level,type,message,status,created_at) SELECT c.id,?,?,?,'OPEN',? FROM chargers c WHERE %1 AND NOT EXISTS(SELECT 1 FROM alarms a WHERE a.charger_id=c.id AND a.type=? AND a.status IN ('OPEN','ACKNOWLEDGED'))").arg(rule.condition));insert.addBindValue(rule.level);insert.addBindValue(rule.type);insert.addBindValue(rule.message);insert.addBindValue(timestamp);insert.addBindValue(rule.type);
        if(!insert.exec()){rollback();if(error)*error=insert.lastError().text();return -1;}created+=qMax(0,insert.numRowsAffected());
    }
    struct Recovery{const char *type;const char *healthy;};const Recovery recoveries[]={
        {"DEVICE_FAULT","c.status!='FAULT'"},{"NETWORK_OFFLINE","c.status!='OFFLINE'"},{"HEARTBEAT_TIMEOUT","c.last_seen IS NULL OR c.status IN ('OFFLINE','FAULT') OR (julianday('now')-julianday(c.last_seen))*86400<=120"}
    };
    for(const Recovery &recovery:recoveries){QSqlQuery resolve(m_db);resolve.prepare(QString("UPDATE alarms SET status='RESOLVED',resolved_at=? WHERE type=? AND status IN ('OPEN','ACKNOWLEDGED') AND EXISTS(SELECT 1 FROM chargers c WHERE c.id=alarms.charger_id AND %1)").arg(recovery.healthy));resolve.addBindValue(timestamp);resolve.addBindValue(recovery.type);if(!resolve.exec()){rollback();if(error)*error=resolve.lastError().text();return -1;}}
    if(!commit(error)){rollback();return -1;}return created;
}

bool Database::acknowledgeAlarm(qint64 alarmId,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE alarms SET status='ACKNOWLEDGED',acknowledged_at=? WHERE id=? AND status='OPEN'");q.addBindValue(now());q.addBindValue(alarmId);if(!q.exec()||q.numRowsAffected()!=1){if(error)*error="告警不存在或已经处理";return false;}QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','ACK_ALARM','ALARM',?,'SUCCESS',?)");log.addBindValue(alarmId);log.addBindValue(now());log.exec();return true;
}

QJsonObject Database::createMaintenance(qint64 alarmId,const QString &assignee,const QString &scheduledAt,QString *error)
{
    const QString worker=assignee.trimmed();if(alarmId<=0||worker.isEmpty()){if(error)*error="请选择告警并填写维修负责人";return{};}if(!begin(error))return{};
    QSqlQuery alarm(m_db);alarm.prepare("SELECT charger_id,message,status FROM alarms WHERE id=?");alarm.addBindValue(alarmId);if(!alarm.exec()||!alarm.next()||alarm.value(2).toString()=="RESOLVED"){rollback();if(error)*error="告警不存在或已经关闭";return{};}const qint64 chargerId=alarm.value(0).toLongLong();
    QSqlQuery duplicate(m_db);duplicate.prepare("SELECT 1 FROM maintenance_orders WHERE alarm_id=?");duplicate.addBindValue(alarmId);if(!duplicate.exec()||duplicate.next()){rollback();if(error)*error="该告警已经生成维修工单";return{};}
    const QString createdAt=now();QSqlQuery insert(m_db);insert.prepare("INSERT INTO maintenance_orders(alarm_id,charger_id,issue,assignee,scheduled_at,status,created_at) VALUES(?,?,?,?,?,'DISPATCHED',?)");insert.addBindValue(alarmId);insert.addBindValue(chargerId);insert.addBindValue(alarm.value(1).toString());insert.addBindValue(worker);insert.addBindValue(scheduledAt.trimmed().isEmpty()?createdAt:scheduledAt.trimmed());insert.addBindValue(createdAt);if(!insert.exec()){rollback();if(error)*error=insert.lastError().text();return{};}const qint64 id=insert.lastInsertId().toLongLong();
    QSqlQuery ack(m_db);ack.prepare("UPDATE alarms SET status='ACKNOWLEDGED',acknowledged_at=COALESCE(acknowledged_at,?) WHERE id=?");ack.addBindValue(createdAt);ack.addBindValue(alarmId);if(!ack.exec()){rollback();if(error)*error=ack.lastError().text();return{};}QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','DISPATCH_MAINTENANCE','MAINTENANCE',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(createdAt);if(!log.exec()||!commit(error)){rollback();return{};}return{{"id",id}};
}

bool Database::updateMaintenanceStatus(qint64 maintenanceId,const QString &status,const QString &result,QString *error)
{
    if(!QStringList({"IN_PROGRESS","COMPLETED"}).contains(status)){if(error)*error="维修工单状态无效";return false;}if(status=="COMPLETED"&&result.trimmed().isEmpty()){if(error)*error="完成维修时必须填写维修结果";return false;}if(!begin(error))return false;
    QSqlQuery find(m_db);find.prepare("SELECT alarm_id,charger_id,status FROM maintenance_orders WHERE id=?");find.addBindValue(maintenanceId);if(!find.exec()||!find.next()){rollback();if(error)*error="维修工单不存在";return false;}const qint64 alarmId=find.value(0).toLongLong(),chargerId=find.value(1).toLongLong();const QString old=find.value(2).toString();if((status=="IN_PROGRESS"&&old!="DISPATCHED")||(status=="COMPLETED"&&old!="IN_PROGRESS")){rollback();if(error)*error="请按照派单、开始维修、完成维修的顺序操作";return false;}
    QSqlQuery update(m_db);if(status=="IN_PROGRESS"){update.prepare("UPDATE maintenance_orders SET status='IN_PROGRESS',started_at=? WHERE id=?");update.addBindValue(now());update.addBindValue(maintenanceId);}else{update.prepare("UPDATE maintenance_orders SET status='COMPLETED',completed_at=?,result=? WHERE id=?");update.addBindValue(now());update.addBindValue(result.trimmed());update.addBindValue(maintenanceId);}if(!update.exec()){rollback();if(error)*error=update.lastError().text();return false;}
    if(status=="COMPLETED"){QSqlQuery alarm(m_db);alarm.prepare("UPDATE alarms SET status='RESOLVED',resolved_at=? WHERE id=?");alarm.addBindValue(now());alarm.addBindValue(alarmId);QSqlQuery charger(m_db);charger.prepare("UPDATE chargers SET status=CASE WHEN status='FAULT' THEN 'IDLE' ELSE status END WHERE id=?");charger.addBindValue(chargerId);if(!alarm.exec()||!charger.exec()){rollback();if(error)*error="关闭告警或恢复设备状态失败";return false;}}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN',?,'MAINTENANCE',?,'SUCCESS',?)");log.addBindValue(status=="COMPLETED"?"COMPLETE_MAINTENANCE":"START_MAINTENANCE");log.addBindValue(maintenanceId);log.addBindValue(now());if(!log.exec()||!commit(error)){rollback();return false;}return true;
}

QJsonObject Database::addStation(const QJsonObject &s,QString *error)
{
    const QString name=s.value("name").toString().trimmed(),address=s.value("address").toString().trimmed();const double lon=s.value("longitude").toDouble(),lat=s.value("latitude").toDouble(),price=s.value("price").toDouble();
    if(name.isEmpty()||address.isEmpty()||lon<-180||lon>180||lat<-90||lat>90||price<=0){if(error)*error="请填写有效的站名、地址、经纬度和基础电价";return{};}
    QSqlQuery q(m_db);q.prepare("INSERT INTO stations(name,address,longitude,latitude,base_price,status) VALUES(?,?,?,?,?,'ONLINE')");q.addBindValue(name);q.addBindValue(address);q.addBindValue(lon);q.addBindValue(lat);q.addBindValue(price);if(!q.exec()){if(error)*error=q.lastError().text();return{};}const qint64 id=q.lastInsertId().toLongLong();QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','ADD_STATION','STATION',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return{{"id",id}};
}

bool Database::updateStation(const QJsonObject &s,QString *error)
{
    const qint64 id=s.value("stationId").toVariant().toLongLong();
    const QString name=s.value("name").toString().trimmed(),address=s.value("address").toString().trimmed(),status=s.value("status").toString();
    const double lon=s.value("longitude").toDouble(),lat=s.value("latitude").toDouble(),price=s.value("price").toDouble();
    if(id<=0||name.isEmpty()||address.isEmpty()||lon<-180||lon>180||lat<-90||lat>90||price<=0||!QStringList({"ONLINE","OFFLINE","MAINTENANCE"}).contains(status)){if(error)*error="请填写有效的电站资料和状态";return false;}
    QSqlQuery q(m_db);q.prepare("UPDATE stations SET name=?,address=?,longitude=?,latitude=?,base_price=?,status=? WHERE id=?");
    q.addBindValue(name);q.addBindValue(address);q.addBindValue(lon);q.addBindValue(lat);q.addBindValue(price);q.addBindValue(status);q.addBindValue(id);
    if(!q.exec()||q.numRowsAffected()!=1){if(error)*error="电站不存在或保存失败："+q.lastError().text();return false;}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','UPDATE_STATION','STATION',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return true;
}

bool Database::deleteStation(qint64 id,QString *error)
{
    QSqlQuery used(m_db);used.prepare("SELECT COUNT(*) FROM chargers WHERE station_id=?");used.addBindValue(id);
    if(!used.exec()||!used.next()){if(error)*error=used.lastError().text();return false;}
    if(used.value(0).toInt()>0){if(error)*error="该电站仍有关联电桩，请先处理电桩后再删除";return false;}
    QSqlQuery q(m_db);q.prepare("DELETE FROM stations WHERE id=?");q.addBindValue(id);
    if(!q.exec()||q.numRowsAffected()!=1){if(error)*error="电站不存在或删除失败："+q.lastError().text();return false;}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','DELETE_STATION','STATION',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return true;
}

QJsonObject Database::addCharger(const QJsonObject &c,QString *error)
{
    const qint64 stationId=c.value("stationId").toVariant().toLongLong();
    const QString code=c.value("code").toString().trimmed().toUpper(),type=c.value("chargerType").toString(),status=c.value("status").toString("IDLE");
    const double power=c.value("power").toDouble();
    if(stationId<=0||!QRegularExpression("^[A-Z0-9_-]{3,32}$").match(code).hasMatch()||!QStringList({"FAST","SLOW"}).contains(type)||power<=0||power>1000||!QStringList({"IDLE","OFFLINE","FAULT"}).contains(status)){if(error)*error="请填写有效的所属电站、设备编号、类型、功率和状态";return{};}
    QSqlQuery station(m_db);station.prepare("SELECT id FROM stations WHERE id=?");station.addBindValue(stationId);if(!station.exec()||!station.next()){if(error)*error="所属电站不存在";return{};}
    QSqlQuery q(m_db);q.prepare("INSERT INTO chargers(station_id,code,type,rated_power,status,total_sessions,total_duration) VALUES(?,?,?,?,?,0,0)");q.addBindValue(stationId);q.addBindValue(code);q.addBindValue(type);q.addBindValue(power);q.addBindValue(status);
    if(!q.exec()){if(error)*error=q.lastError().nativeErrorCode()=="2067"?"设备编号已存在":q.lastError().text();return{};}
    const qint64 id=q.lastInsertId().toLongLong();QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','ADD_CHARGER','CHARGER',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return{{"id",id}};
}

bool Database::updateCharger(const QJsonObject &c,QString *error)
{
    const qint64 id=c.value("chargerId").toVariant().toLongLong(),stationId=c.value("stationId").toVariant().toLongLong();
    const QString code=c.value("code").toString().trimmed().toUpper(),type=c.value("chargerType").toString(),status=c.value("status").toString();const double power=c.value("power").toDouble();
    if(id<=0||stationId<=0||!QRegularExpression("^[A-Z0-9_-]{3,32}$").match(code).hasMatch()||!QStringList({"FAST","SLOW"}).contains(type)||power<=0||power>1000||!QStringList({"IDLE","OFFLINE","FAULT"}).contains(status)){if(error)*error="请填写有效的电桩资料";return false;}
    QSqlQuery current(m_db);current.prepare("SELECT status FROM chargers WHERE id=?");current.addBindValue(id);if(!current.exec()||!current.next()){if(error)*error="电桩不存在";return false;}if(QStringList({"CHARGING","RESERVED"}).contains(current.value(0).toString())){if(error)*error="正在充电或已预约的电桩不能修改";return false;}
    QSqlQuery q(m_db);q.prepare("UPDATE chargers SET station_id=?,code=?,type=?,rated_power=?,status=? WHERE id=?");q.addBindValue(stationId);q.addBindValue(code);q.addBindValue(type);q.addBindValue(power);q.addBindValue(status);q.addBindValue(id);
    if(!q.exec()||q.numRowsAffected()!=1){if(error)*error=q.lastError().nativeErrorCode()=="2067"?"设备编号已存在":q.lastError().text();return false;}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','UPDATE_CHARGER','CHARGER',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return true;
}

bool Database::deleteCharger(qint64 id,QString *error)
{
    QSqlQuery current(m_db);current.prepare("SELECT status FROM chargers WHERE id=?");current.addBindValue(id);if(!current.exec()||!current.next()){if(error)*error="电桩不存在";return false;}if(QStringList({"CHARGING","RESERVED"}).contains(current.value(0).toString())){if(error)*error="正在充电或已预约的电桩不能删除";return false;}
    QSqlQuery used(m_db);used.prepare("SELECT (SELECT COUNT(*) FROM charge_orders WHERE charger_id=?)+(SELECT COUNT(*) FROM reservations WHERE charger_id=?)+(SELECT COUNT(*) FROM telemetry WHERE charger_id=?)");used.addBindValue(id);used.addBindValue(id);used.addBindValue(id);if(!used.exec()||!used.next()){if(error)*error=used.lastError().text();return false;}if(used.value(0).toInt()>0){if(error)*error="该电桩已有订单、预约或遥测记录，为保留历史数据不能删除；可将状态改为 OFFLINE";return false;}
    QSqlQuery q(m_db);q.prepare("DELETE FROM chargers WHERE id=?");q.addBindValue(id);if(!q.exec()||q.numRowsAffected()!=1){if(error)*error=q.lastError().text();return false;}
    QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','DELETE_CHARGER','CHARGER',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return true;
}

bool Database::setUserStatus(qint64 id,const QString &status,QString *error)
{
    if(!QStringList({"NORMAL","FROZEN","LOCKED"}).contains(status)){if(error)*error="无效用户状态";return false;}QSqlQuery q(m_db);q.prepare("UPDATE users SET status=?,failed_attempts=CASE WHEN ?='NORMAL' THEN 0 ELSE failed_attempts END,locked_at=CASE WHEN ?='NORMAL' THEN NULL ELSE locked_at END WHERE id=?");q.addBindValue(status);q.addBindValue(status);q.addBindValue(status);q.addBindValue(id);if(!q.exec()||q.numRowsAffected()!=1){if(error)*error="用户不存在";return false;}QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN',?,'USER',?,'SUCCESS',?)");log.addBindValue(status=="FROZEN"?"FREEZE_USER":status=="LOCKED"?"LOCK_USER":"UNLOCK_USER");log.addBindValue(id);log.addBindValue(now());log.exec();return true;
}

bool Database::restartCharger(qint64 id,QString *error)
{
    QSqlQuery q(m_db);q.prepare("UPDATE chargers SET status='IDLE',last_seen=? WHERE id=? AND status!='CHARGING'");q.addBindValue(now());q.addBindValue(id);if(!q.exec()||q.numRowsAffected()!=1){if(error)*error="设备不存在或正在充电，不能重启";return false;}QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','RESTART_CHARGER','CHARGER',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return true;
}

int Database::expireReservations(QString *error)
{
    if(!begin(error))return -1;
    QSqlQuery find(m_db);
    find.prepare("SELECT id,charger_id FROM reservations WHERE status='ACTIVE' AND expires_at<=?");
    find.addBindValue(now());
    if(!find.exec()){rollback();if(error)*error=find.lastError().text();return -1;}
    QList<qint64> ids,chargers;
    while(find.next()){ids.append(find.value(0).toLongLong());chargers.append(find.value(1).toLongLong());}
    QSqlQuery cancel(m_db),release(m_db);
    for(int i=0;i<ids.size();++i){
        cancel.prepare("UPDATE reservations SET status='CANCELLED',cancel_reason='TIMEOUT' WHERE id=? AND status='ACTIVE'");cancel.addBindValue(ids[i]);
        if(!cancel.exec()){rollback();if(error)*error=cancel.lastError().text();return -1;}
        release.prepare("UPDATE chargers SET status='IDLE' WHERE id=? AND status='RESERVED'");release.addBindValue(chargers[i]);
        if(!release.exec()){rollback();if(error)*error=release.lastError().text();return -1;}
    }
    if(!commit(error)){rollback();return -1;}return ids.size();
}
