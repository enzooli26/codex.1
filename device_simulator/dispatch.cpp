#include "simulator.h"
#include "framecodec.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>

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
