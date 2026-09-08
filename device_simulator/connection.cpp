#include "simulator.h"
#include "framecodec.h"
#include "simulatortick.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>
#include <QUuid>

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
