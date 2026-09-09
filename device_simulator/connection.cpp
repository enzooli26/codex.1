#include "simulator.h"
#include "framecodec.h"
#include "simulatortick.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>
#include <QUuid>

// 网络连上后：启动定时器、发送注册请求
void Simulator::onNetworkConnected()
{
    m_registered = false;
    if(m_tick) QMetaObject::invokeMethod(m_tick, "start", Qt::QueuedConnection);
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

// 网络断开：停止定时器、标记断开状态、检测待付款订单
void Simulator::onNetworkDisconnected()
{
    m_registered = false;
    if(m_tick) QMetaObject::invokeMethod(m_tick, "stopHeartbeat", Qt::QueuedConnection);
    qWarning() << "central server disconnected; local charging and metering continue";
    emit connectionChanged(false);
    m_disconnected = true;
    emit disconnectedStateChanged(m_disconnected);
    m_heartbeatFailures = 0;
    checkDisconnection();
}

// 心跳超时：累计 3 次超时判定为网络断开
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

// 将所有待付款订单标记为断网状态（记录 disconnected_at）
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
