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
        {"admins","locked_at","TEXT"}
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
                      "COUNT(c.id),SUM(CASE WHEN c.status='IDLE' THEN 1 ELSE 0 END),"
                      "MIN(CASE WHEN c.status='IDLE' THEN c.id END) "
                      "FROM stations s LEFT JOIN chargers c ON c.station_id=s.id "
                      "GROUP BY s.id ORDER BY s.id";
    if (!q.exec(sql)) { if (error) *error=q.lastError().text(); return {}; }
    QJsonArray result;
    while(q.next()) result.append(QJsonObject{{"id",q.value(0).toLongLong()},{"name",q.value(1).toString()},
        {"address",q.value(2).toString()},{"longitude",q.value(3).toDouble()},
        {"latitude",q.value(4).toDouble()},{"price",q.value(5).toDouble()},
        {"total",q.value(6).toInt()},{"idle",q.value(7).toInt()},
        {"chargerId",q.value(8).toLongLong()}});
    return result;
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
    if (target<=0 || !QStringList({"AMOUNT","ENERGY","TIME"}).contains(mode)) { if(error)*error="充电目标无效"; return {}; }
    if(!begin(error))return{};
    QSqlQuery user(m_db);user.prepare("SELECT balance,status FROM users WHERE id=?");user.addBindValue(userId);
    if(!user.exec()||!user.next()||user.value(1).toString()!="NORMAL"){rollback();if(error)*error="用户状态异常";return{};}
    const double balance=user.value(0).toDouble();
    double minRequired=0;
    if(mode=="AMOUNT"){
        minRequired=target;
    }else if(mode=="ENERGY"){
        QSqlQuery price(m_db);price.prepare("SELECT s.base_price FROM chargers c JOIN stations s ON s.id=c.station_id WHERE c.id=?");price.addBindValue(chargerId);
        if(!price.exec()||!price.next()){rollback();if(error)*error="无法获取电价";return{};}
        minRequired=target*price.value(0).toDouble();
    }
    if(balance<=0||balance<minRequired){rollback();if(error)*error=QString("余额不足，当前余额 ¥%1，需要 ¥%2").arg(balance,0,'f',2).arg(minRequired,0,'f',2);return{};}
    QSqlQuery busy(m_db);busy.prepare("SELECT 1 FROM charge_orders WHERE user_id=? AND status IN ('CHARGING','PENDING_PAYMENT')");busy.addBindValue(userId);
    if(!busy.exec()||busy.next()){rollback();if(error)*error="存在未完成订单";return{};}
    QSqlQuery lock(m_db);lock.prepare("UPDATE chargers SET status='CHARGING' WHERE id=? AND status IN ('IDLE','RESERVED')");lock.addBindValue(chargerId);
    if(!lock.exec()||lock.numRowsAffected()!=1){rollback();if(error)*error="充电桩不可用";return{};}
    QSqlQuery cancel(m_db);cancel.prepare("UPDATE reservations SET status='USED' WHERE user_id=? AND charger_id=? AND status='ACTIVE'");cancel.addBindValue(userId);cancel.addBindValue(chargerId);cancel.exec();
    QSqlQuery insert(m_db);insert.prepare("INSERT INTO charge_orders(user_id,charger_id,status,mode,target,start_at,energy,duration,amount) VALUES(?,?,'CHARGING',?,?,?,0,0,0)");
    insert.addBindValue(userId);insert.addBindValue(chargerId);insert.addBindValue(mode);insert.addBindValue(target);insert.addBindValue(now());
    if(!insert.exec()||!commit(error)){rollback();if(error&&error->isEmpty())*error=insert.lastError().text();return{};}
    return {{"orderId",insert.lastInsertId().toLongLong()},{"status","CHARGING"}};
}

QJsonObject Database::stopCharge(qint64 userId, qint64 orderId, QString *error)
{
    if(!begin(error))return{};
    QSqlQuery find(m_db);find.prepare("SELECT charger_id,energy,start_at FROM charge_orders WHERE id=? AND user_id=? AND status='CHARGING'");find.addBindValue(orderId);find.addBindValue(userId);
    if(!find.exec()||!find.next()){rollback();if(error)*error="活动订单不存在";return{};}
    const qint64 chargerId=find.value(0).toLongLong(); const double energy=find.value(1).toDouble();
    const int elapsed=qMax(1,static_cast<int>(QDateTime::fromString(find.value(2).toString(),Qt::ISODate).secsTo(QDateTime::currentDateTimeUtc())));
    QSqlQuery price(m_db);price.prepare("SELECT s.base_price FROM chargers c JOIN stations s ON s.id=c.station_id WHERE c.id=?");price.addBindValue(chargerId);
    if(!price.exec()||!price.next()){rollback();if(error)*error="无法获取电价";return{};}
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
    q.prepare("UPDATE chargers SET status=CASE WHEN status IN ('RESERVED','CHARGING') AND ?='IDLE' THEN status ELSE ? END,last_seen=? WHERE code=?");
    q.addBindValue(status);q.addBindValue(status);q.addBindValue(now());q.addBindValue(chargerCode);
    if(!q.exec()){if(error)*error=q.lastError().text();return false;}return q.numRowsAffected()==1;
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

QJsonObject Database::adminSummary(QString *error)
{
    QJsonObject result;
    QSqlQuery q(m_db);
    if(!q.exec("SELECT COUNT(*),SUM(CASE WHEN status='IDLE' THEN 1 ELSE 0 END),SUM(CASE WHEN status='CHARGING' THEN 1 ELSE 0 END),SUM(CASE WHEN status='FAULT' THEN 1 ELSE 0 END) FROM chargers")||!q.next()){if(error)*error=q.lastError().text();return{};}
    result["chargers"]=q.value(0).toInt();result["idle"]=q.value(1).toInt();result["charging"]=q.value(2).toInt();result["fault"]=q.value(3).toInt();
    QSqlQuery metrics(m_db);
    if(!metrics.exec("SELECT COALESCE(SUM(CASE WHEN date(end_at)=date('now','localtime') THEN amount END),0),"
                     "COALESCE(SUM(CASE WHEN strftime('%Y-%m',end_at)=strftime('%Y-%m','now','localtime') THEN amount END),0),"
                     "COALESCE(SUM(amount),0),SUM(CASE WHEN date(start_at)=date('now','localtime') THEN 1 ELSE 0 END) "
                     "FROM charge_orders WHERE status='COMPLETED'")||!metrics.next()){if(error)*error=metrics.lastError().text();return{};}
    result["todayRevenue"]=metrics.value(0).toDouble();result["monthRevenue"]=metrics.value(1).toDouble();
    result["revenue"]=metrics.value(2).toDouble();result["todayOrders"]=metrics.value(3).toInt();
    QSqlQuery users(m_db);if(users.exec("SELECT COUNT(*) FROM users")&&users.next())result["users"]=users.value(0).toInt();
    QJsonArray trend;
    QSqlQuery tq(m_db);tq.exec("WITH RECURSIVE days(d) AS (SELECT date('now','localtime','-6 day') UNION ALL SELECT date(d,'+1 day') FROM days WHERE d<date('now','localtime')) SELECT d,COALESCE(SUM(o.amount),0) FROM days LEFT JOIN charge_orders o ON date(o.end_at)=d AND o.status='COMPLETED' GROUP BY d ORDER BY d");
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
    QSqlQuery q(m_db);q.prepare("SELECT c.id,c.code,s.name,c.type,c.rated_power,c.status,c.total_sessions,c.total_duration,COALESCE(c.last_seen,'--') FROM chargers c JOIN stations s ON s.id=c.station_id ORDER BY c.id DESC");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"code",q.value(1).toString()},{"station",q.value(2).toString()},{"chargerType",q.value(3).toString()},{"power",q.value(4).toDouble()},{"status",q.value(5).toString()},{"sessions",q.value(6).toInt()},{"duration",q.value(7).toInt()},{"lastSeen",q.value(8).toString()}});return a;
}

QJsonArray Database::adminOrders(QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT o.id,u.phone,s.name,c.code,o.status,o.mode,o.target,o.energy,o.duration,o.amount,o.start_at,COALESCE(o.end_at,'--') FROM charge_orders o JOIN users u ON u.id=o.user_id JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id ORDER BY o.id DESC LIMIT 300");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"phone",q.value(1).toString()},{"station",q.value(2).toString()},{"charger",q.value(3).toString()},{"status",q.value(4).toString()},{"mode",q.value(5).toString()},{"target",q.value(6).toDouble()},{"energy",q.value(7).toDouble()},{"duration",q.value(8).toInt()},{"amount",q.value(9).toDouble()},{"startAt",q.value(10).toString()},{"endAt",q.value(11).toString()}});return a;
}

QJsonArray Database::adminUsers(const QString &phoneFilter,QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT id,phone,nickname,balance,status,failed_attempts,COALESCE(locked_at,'--'),created_at FROM users WHERE phone LIKE ? ORDER BY id DESC LIMIT 300");q.addBindValue("%"+phoneFilter+"%");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"phone",q.value(1).toString()},{"nickname",q.value(2).toString()},{"balance",q.value(3).toDouble()},{"status",q.value(4).toString()},{"failedAttempts",q.value(5).toInt()},{"lockedAt",q.value(6).toString()},{"createdAt",q.value(7).toString()}});return a;
}

QJsonArray Database::adminLogs(QString *error)
{
    QSqlQuery q(m_db);q.prepare("SELECT id,COALESCE(actor_type,'SYSTEM'),COALESCE(actor_id,0),action,COALESCE(target_type,'--'),COALESCE(target_id,0),COALESCE(result,'--'),created_at FROM operation_logs ORDER BY id DESC LIMIT 300");
    if(!q.exec()){if(error)*error=q.lastError().text();return{};}QJsonArray a;while(q.next())a.append(QJsonObject{{"id",q.value(0).toLongLong()},{"actor",q.value(1).toString()+"#"+q.value(2).toString()},{"action",q.value(3).toString()},{"target",q.value(4).toString()+"#"+q.value(5).toString()},{"result",q.value(6).toString()},{"createdAt",q.value(7).toString()}});return a;
}

QJsonObject Database::addStation(const QJsonObject &s,QString *error)
{
    const QString name=s.value("name").toString().trimmed(),address=s.value("address").toString().trimmed();const double lon=s.value("longitude").toDouble(),lat=s.value("latitude").toDouble(),price=s.value("price").toDouble();
    if(name.isEmpty()||address.isEmpty()||lon<-180||lon>180||lat<-90||lat>90||price<=0){if(error)*error="请填写有效的站名、地址、经纬度和基础电价";return{};}
    QSqlQuery q(m_db);q.prepare("INSERT INTO stations(name,address,longitude,latitude,base_price,status) VALUES(?,?,?,?,?,'ONLINE')");q.addBindValue(name);q.addBindValue(address);q.addBindValue(lon);q.addBindValue(lat);q.addBindValue(price);if(!q.exec()){if(error)*error=q.lastError().text();return{};}const qint64 id=q.lastInsertId().toLongLong();QSqlQuery log(m_db);log.prepare("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','ADD_STATION','STATION',?,'SUCCESS',?)");log.addBindValue(id);log.addBindValue(now());log.exec();return{{"id",id}};
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

int Database::markAbnormalOrders(QString *error)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE chargers SET status='IDLE' WHERE status IN ('CHARGING','RESERVED')");
    if(!q.exec()){if(error)*error=q.lastError().text();return -1;}
    QSqlQuery o(m_db);
    o.prepare("UPDATE charge_orders SET status='ABNORMAL',end_at=? WHERE status='CHARGING'");
    o.addBindValue(now());
    if(!o.exec()){if(error)*error=o.lastError().text();return -1;}
    return o.numRowsAffected();
}

QJsonArray Database::autoCompleteDisconnectedOrders(qint64 userId, QString *error)
{
    if(!begin(error))return{};
    QSqlQuery find(m_db);
    find.prepare("SELECT id,charger_id FROM charge_orders WHERE user_id=? AND status='CHARGING'");
    find.addBindValue(userId);
    if(!find.exec()){rollback();if(error)*error=find.lastError().text();return{};}
    QJsonArray completed;
    while(find.next()){
        const qint64 orderId=find.value(0).toLongLong();
        const qint64 chargerId=find.value(1).toLongLong();
        QSqlQuery upd(m_db);
        upd.prepare("UPDATE charge_orders SET status='ABNORMAL',end_at=? WHERE id=?");
        upd.addBindValue(now()); upd.addBindValue(orderId);
        if(!upd.exec()){rollback();if(error)*error=upd.lastError().text();return{};}
        QSqlQuery release(m_db);
        release.prepare("UPDATE chargers SET status='IDLE' WHERE id=? AND status='CHARGING'");
        release.addBindValue(chargerId);
        release.exec();
        completed.append(QJsonObject{{"orderId",orderId}});
    }
    if(!commit(error)){rollback();return{};}
    return completed;
}
