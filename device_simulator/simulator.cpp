#include "simulator.h"
#include "devicenetwork.h"
#include "framecodec.h"
#include "simulatortick.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>
#include <QRandomGenerator>
#include <QSet>
#include <QUuid>

Simulator::Simulator(const QStringList &codes, const QString &databasePath,
                     const QString &token, QObject *parent)
    : QObject(parent), m_codes(codes), m_databasePath(databasePath), m_token(token)
{
    for(const QString &code : m_codes) m_soc[code] = 35.0;
}

void Simulator::setNetwork(DeviceNetwork *network)
{
    m_network = network;
    connect(this, &Simulator::connectNetwork, network, &DeviceNetwork::connectToHost);
    connect(this, &Simulator::disconnectNetwork, network, &DeviceNetwork::disconnectFromHost);
    connect(this, &Simulator::sendMessage, network, &DeviceNetwork::sendMessage);
    connect(network, &DeviceNetwork::connected, this, &Simulator::onNetworkConnected);
    connect(network, &DeviceNetwork::disconnected, this, &Simulator::onNetworkDisconnected);
    connect(network, &DeviceNetwork::messageReceived, this, &Simulator::onMessageReceived);
}

void Simulator::setDatabase(EdgeDatabase *database)
{
    m_database = database;
}

void Simulator::setTick(SimulatorTick *tick)
{
    m_tick = tick;
    connect(tick, &SimulatorTick::heartbeatTick, this, &Simulator::onHeartbeatTick);
    connect(tick, &SimulatorTick::telemetryTick, this, &Simulator::onTelemetryTick);
    connect(tick, &SimulatorTick::heartbeatTimeout, this, &Simulator::onHeartbeatTimeout);
}

bool Simulator::initialize(QString *error)
{
    bool ok = false;
    QMetaObject::invokeMethod(m_database, [this, &ok, error]() {
        ok = m_database->open(m_databasePath, m_codes, error);
    }, Qt::BlockingQueuedConnection);
    return ok;
}

void Simulator::start(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    emit connectNetwork(host, port);
}

void Simulator::disconnectFromServer()
{
    m_manualDisconnect = true;
    m_registered = false;
    m_heartbeatFailures = 0;
    if(m_tick) m_tick->stop();
    emit disconnectNetwork();
    qInfo() << "simulator manually disconnected; local charging continues";
    emit connectionChanged(false);
    m_disconnected = true;
    emit disconnectedStateChanged(m_disconnected);
}

void Simulator::connectToServer()
{
    m_manualDisconnect = false;
    emit connectNetwork(m_host, m_port);
}

QJsonArray Simulator::stations(QString *error)
{
    QJsonArray result;
    QMetaObject::invokeMethod(m_database, [this, &result, error]() {
        result = m_database->stations(error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

QJsonArray Simulator::chargersByStation(int stationId, QString *error)
{
    QJsonArray result;
    QMetaObject::invokeMethod(m_database, [this, stationId, &result, error]() {
        result = m_database->chargersByStation(stationId, error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

QJsonObject Simulator::activeOrderForCharger(const QString &code, QString *error)
{
    QJsonObject result;
    QMetaObject::invokeMethod(m_database, [this, &code, &result, error]() {
        result = m_database->activeOrderForCharger(code, error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

QString Simulator::chargerStatus(const QString &code, QString *error)
{
    QString result;
    QMetaObject::invokeMethod(m_database, [this, code, &result, error]() {
        result = m_database->chargerStatus(code, error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

void Simulator::onNetworkConnected()
{
    m_registered = false;
    if(m_tick) m_tick->start();
    QString error;
    QJsonArray chargers;
    QMetaObject::invokeMethod(m_database, [this, &chargers, &error]() {
        chargers = m_database->chargers(&error);
    }, Qt::BlockingQueuedConnection);
    sendRequest("device.register", {{"token", m_token}, {"chargers", chargers}});
    qInfo() << "charger edge TLS connected; local database" << m_databasePath;
    emit connectionChanged(true);
    m_disconnected = false;
    emit disconnectedStateChanged(m_disconnected);
}

void Simulator::onNetworkDisconnected()
{
    m_registered = false;
    if(m_tick) m_tick->stop();
    qWarning() << "central server disconnected; local charging and metering continue";
    emit connectionChanged(false);
    m_disconnected = true;
    emit disconnectedStateChanged(m_disconnected);
    m_heartbeatFailures = 0;
    checkDisconnection();
}

void Simulator::onHeartbeatTimeout()
{
    m_heartbeatFailures++;
    qWarning() << "heartbeat timeout, failures:" << m_heartbeatFailures;
    if(m_heartbeatFailures >= 3) {
        qWarning() << "network disconnected detected after 3 heartbeat timeouts";
        m_disconnected = true;
        emit disconnectedStateChanged(m_disconnected);
        checkDisconnection();
    }
}

void Simulator::checkDisconnection()
{
    QString error;
    QJsonArray orders;
    QMetaObject::invokeMethod(m_database, [this, &orders, &error]() {
        orders = m_database->pendingOrders(&error);
    }, Qt::BlockingQueuedConnection);
    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    for(int i = 0; i < orders.size(); ++i) {
        const QJsonObject order = orders[i].toObject();
        const qint64 orderId = order.value("orderId").toVariant().toLongLong();
        bool ok = false;
        QMetaObject::invokeMethod(m_database, [this, orderId, &ok, &error]() {
            ok = m_database->updateOrderPaymentStatus(orderId, "PENDING", &error);
        }, Qt::BlockingQueuedConnection);
        if(ok) {
            QMetaObject::invokeMethod(m_database, [this, orderId, now, &error]() {
                m_database->updateOrderDisconnectedAt(orderId, now, &error);
            }, Qt::BlockingQueuedConnection);
        }
    }
}

void Simulator::onHeartbeatTick()
{
    if(!m_registered) return;
    QJsonArray orders;
    QJsonArray allChargers;
    QString error;
    QMetaObject::invokeMethod(m_database, [this, &allChargers, &error]() {
        allChargers = m_database->chargers(&error);
    }, Qt::BlockingQueuedConnection);
    for(const auto &v : allChargers) {
        const QString code = v.toObject().value("code").toString();
        QJsonObject active;
        QMetaObject::invokeMethod(m_database, [this, code, &active, &error]() {
            active = m_database->activeOrderForCharger(code, &error);
        }, Qt::BlockingQueuedConnection);
        if(!active.isEmpty()) {
            orders.append(QJsonObject{
                {"chargerCode", code},
                {"status", active.value("status").toString()},
                {"orderId", active.value("orderId").toVariant().toLongLong()}
            });
        }
    }
    sendRequest("device.heartbeat", {{"status", "connected"}, {"orders", orders}});
    if(m_tick) m_tick->resetHeartbeatTimer();
    m_heartbeatFailures = 0;
}

void Simulator::onTelemetryTick()
{
    bool completed = false;
    QString err;
    QJsonArray allChargers;
    QMetaObject::invokeMethod(m_database, [this, &allChargers, &err]() {
        allChargers = m_database->chargers(&err);
    }, Qt::BlockingQueuedConnection);
    for(const auto &v : allChargers) {
        const QString code = v.toObject().value("code").toString();
        QString error;
        QString status;
        QMetaObject::invokeMethod(m_database, [this, code, &status, &error]() {
            status = m_database->chargerStatus(code, &error);
        }, Qt::BlockingQueuedConnection);
        if(status != "CHARGING") continue;
        const double voltage = 380.0 + QRandomGenerator::global()->bounded(500) / 100.0;
        double ratedPower = 0;
        QMetaObject::invokeMethod(m_database, [this, code, &ratedPower, &error]() {
            ratedPower = m_database->chargerRatedPower(code, &error);
        }, Qt::BlockingQueuedConnection);
        const double power = ratedPower * (0.85 + QRandomGenerator::global()->bounded(151) / 1000.0);
        const double current = power * 1000.0 / voltage;
        m_soc[code] = qMin(100.0, m_soc.value(code, 35.0) + 0.05);
        QJsonObject order;
        QMetaObject::invokeMethod(m_database, [this, code, voltage, current, power, &order, &error]() {
            order = m_database->tick(code, voltage, current, power, m_soc.value(code), &error);
        }, Qt::BlockingQueuedConnection);
        if(!error.isEmpty()) { qWarning() << error; continue; }
        if(!order.isEmpty()) emit orderChanged(code, order);
        if(m_registered) {
            QJsonObject telemetryPayload = {
                {"chargerCode", code}, {"voltage", voltage},
                {"current", current}, {"power", power}, {"soc", m_soc.value(code)}
            };
            sendRequest("device.telemetry", telemetryPayload);
        }
        if(order.value("status").toString() == "SYNC_PENDING") completed = true;
    }
    if(completed && m_registered) sendSync();
}

void Simulator::sendRequest(const QString &type, const QJsonObject &payload)
{
    emit sendMessage(Protocol::request(type, payload,
        QUuid::createUuid().toString(QUuid::WithoutBraces)));
}

void Simulator::sendSync()
{
    if(!m_registered) return;
    QString error;
    QJsonArray orders;
    QMetaObject::invokeMethod(m_database, [this, &orders, &error]() {
        orders = m_database->pendingOrders(&error);
    }, Qt::BlockingQueuedConnection);
    if(error.isEmpty())
        sendRequest("device.sync", {{"orders", orders}});
    else
        qWarning() << error;
}

void Simulator::syncPendingOrders()
{
    if(!m_registered) return;
    QString error;
    QJsonArray orders;
    QMetaObject::invokeMethod(m_database, [this, &orders, &error]() {
        orders = m_database->pendingPaymentOrders(&error);
    }, Qt::BlockingQueuedConnection);
    if(!error.isEmpty()) { qWarning() << error; return; }
    if(orders.isEmpty()) return;
    QJsonArray syncOrders;
    for(int i = 0; i < orders.size(); ++i) {
        const QJsonObject o = orders[i].toObject();
        if(o.value("status").toString() != "SYNC_PENDING") continue;
        syncOrders.append(QJsonObject{
            {"orderId", o.value("centralOrderId").toVariant().toLongLong()},
            {"chargerCode", o.value("chargerCode").toString()},
            {"status", o.value("status").toString()},
            {"energy", o.value("energy").toDouble()},
            {"duration", o.value("duration").toInt()},
            {"endAt", o.value("endAt").toString()}
        });
    }
    if(syncOrders.isEmpty()) return;
    qInfo() << "syncing" << syncOrders.size() << "disconnected orders to server";
    sendRequest("device.sync", {{"orders", syncOrders}});
}

void Simulator::stopOrder(const QString &chargerCode)
{
    QString error;
    QJsonObject active;
    QMetaObject::invokeMethod(m_database, [this, chargerCode, &active, &error]() {
        active = m_database->activeOrderForCharger(chargerCode, &error);
    }, Qt::BlockingQueuedConnection);
    if(active.isEmpty()) return;
    const qint64 orderId = active.value("orderId").toVariant().toLongLong();
    QJsonObject result;
    QMetaObject::invokeMethod(m_database, [this, orderId, &result, &error]() {
        result = m_database->stopOrder(orderId, &error);
    }, Qt::BlockingQueuedConnection);
    if(!result.isEmpty()) {
        emit chargerStatusChanged(chargerCode, "IDLE");
        emit orderChanged(chargerCode, result);
    }
    if(m_registered) sendSync();
}

void Simulator::syncChargerList(const QJsonArray &serverChargers)
{
    QString err;
    QJsonArray localChargers;
    QMetaObject::invokeMethod(m_database, [this, &localChargers, &err]() {
        localChargers = m_database->chargers(&err);
    }, Qt::BlockingQueuedConnection);
    QSet<QString> localCodes;
    for(const auto &v : localChargers)
        localCodes.insert(v.toObject().value("code").toString());
    for(const auto &v : serverChargers) {
        const QJsonObject c = v.toObject();
        const QString code = c.value("code").toString();
        if(code.isEmpty() || localCodes.contains(code)) continue;
        bool ok = false;
        QMetaObject::invokeMethod(m_database, [this, code, c, &ok, &err]() {
            ok = m_database->addChargerFromServer(
                code,
                c.value("type").toString("FAST"),
                c.value("ratedPower").toDouble(120),
                c.value("stationName").toString("未分配站点"),
                &err);
        }, Qt::BlockingQueuedConnection);
        if(ok) {
            m_soc[code] = 35.0;
            emit chargerAdded(code);
            qInfo() << "Synced new charger from server:" << code;
        } else {
            qWarning() << "Failed to sync charger" << code << ":" << err;
        }
    }
}

void Simulator::onMessageReceived(QJsonObject message)
{
    dispatch(message);
}

void Simulator::dispatch(const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    const QJsonObject payload = message.value("payload").toObject();
    QString error;
    QJsonObject data;

    if(type == "device.register.result") {
        if(message.value("code").toInt() != 0) {
            qWarning() << "device registration rejected" << message.value("message").toString();
            emit disconnectNetwork();
            return;
        }
        m_registered = true;
        const QJsonArray serverChargers = message.value("data").toObject().value("chargers").toArray();
        syncChargerList(serverChargers);
        onHeartbeatTick();
        sendSync();
        syncPendingOrders();
        return;
    }
    if(type == "device.sync.result") {
        if(message.value("code").toInt() != 0) {
            qWarning() << "sync rejected" << message.value("message").toString();
            return;
        }
        for(const auto &value : message.value("data").toObject().value("results").toArray()) {
            const QJsonObject item = value.toObject();
            if(item.value("code").toInt() == 0 && item.value("status").toString() == "COMPLETED") {
                QMetaObject::invokeMethod(m_database, [this, item, &error]() {
                    m_database->settleOrder(item.value("orderId").toVariant().toLongLong(), &error);
                }, Qt::BlockingQueuedConnection);
            }
        }
        emit syncCompleted();
        return;
    }
    if(type == "device.order.start") {
        QMetaObject::invokeMethod(m_database, [this, payload, &data, &error]() {
            data = m_database->startOrder(payload, &error);
        }, Qt::BlockingQueuedConnection);
        if(error.isEmpty()) {
            const QString code = payload.value("chargerCode").toString();
            emit chargerStatusChanged(code, "CHARGING");
            emit orderChanged(code, data);
        }
    }
    else if(type == "device.order.stop") {
        QMetaObject::invokeMethod(m_database, [this, payload, &data, &error]() {
            data = m_database->stopOrder(payload.value("orderId").toVariant().toLongLong(), &error);
        }, Qt::BlockingQueuedConnection);
        if(error.isEmpty() || data.value("status").toString() == "SYNC_PENDING") {
            const QString code = payload.value("chargerCode").toString();
            if(!code.isEmpty()) {
                emit chargerStatusChanged(code, "IDLE");
                emit orderChanged(code, data);
            }
        }
    }
    else if(type == "device.order.start.result") {
        if(message.value("code").toInt() == 0) {
            const qint64 localOrderId = payload.value("orderId").toVariant().toLongLong();
            const qint64 serverOrderId = payload.value("serverOrderId").toVariant().toLongLong();
            if(serverOrderId > 0) {
                QMetaObject::invokeMethod(m_database, [this, localOrderId, serverOrderId, &error]() {
                    m_database->updateServerOrderId(localOrderId, serverOrderId, &error);
                }, Qt::BlockingQueuedConnection);
            }
        }
    }
    else if(type == "device.order.settled") {
        bool settled = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &settled, &error]() {
            settled = m_database->settleOrder(payload.value("orderId").toVariant().toLongLong(), &error);
        }, Qt::BlockingQueuedConnection);
        if(settled) data = {{"settled", true}};
    }
    else if(type == "device.order.abort") {
        bool aborted = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &aborted, &error]() {
            aborted = m_database->abortOrder(payload.value("orderId").toVariant().toLongLong(), &error);
        }, Qt::BlockingQueuedConnection);
        if(aborted) data = {{"aborted", true}};
    }
    else if(type == "device.disconnected.sync.result") {
        const QJsonArray results = payload.value("results").toArray();
        for(const auto &value : results) {
            const QJsonObject item = value.toObject();
            if(item.value("code").toInt() == 0) {
                const qint64 orderId = item.value("orderId").toVariant().toLongLong();
                QMetaObject::invokeMethod(m_database, [this, orderId, &error]() {
                    m_database->updateOrderPaymentStatus(orderId, "PAID", &error);
                }, Qt::BlockingQueuedConnection);
            }
        }
        return;
    }
    else if(type == "device.charger.added") {
        bool added = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &added, &error]() {
            added = m_database->addChargerFromServer(
                payload.value("code").toString(),
                payload.value("type").toString(),
                payload.value("ratedPower").toDouble(),
                payload.value("stationName").toString(),
                &error);
        }, Qt::BlockingQueuedConnection);
        if(added) {
            const QString newCode = payload.value("code").toString();
            m_soc[newCode] = 35.0;
            emit chargerAdded(newCode);
            qInfo() << "New charger synced from server:" << newCode;
        } else {
            qWarning() << "Failed to sync charger:" << error;
        }
        return;
    }
    else if(type == "device.charger.removed") {
        bool removed = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &removed, &error]() {
            removed = m_database->removeChargerByCode(payload.value("code").toString(), &error);
        }, Qt::BlockingQueuedConnection);
        if(removed) {
            emit chargerRemoved(payload.value("code").toString());
            qInfo() << "Charger removed from server:" << payload.value("code").toString();
        }
        return;
    }
    else return;
    emit sendMessage(Protocol::response(message, error.isEmpty() ? 0 : 400,
                                         error.isEmpty() ? "ok" : error, data));
}
