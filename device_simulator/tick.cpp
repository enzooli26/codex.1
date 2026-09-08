#include "simulator.h"
#include "framecodec.h"
#include "simulatortick.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>
#include <QRandomGenerator>
#include <QUuid>

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
