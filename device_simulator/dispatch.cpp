#include "simulator.h"
#include "framecodec.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>

// 收到网络消息后转入分发器
void Simulator::onMessageReceived(QJsonObject message)
{
    dispatch(message);
}

// 根据消息 type 分发到对应处理逻辑
void Simulator::dispatch(const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    const QJsonObject payload = message.contains("payload")
        ? message.value("payload").toObject()
        : message.value("data").toObject();
    QString error;
    QJsonObject data;

    // 设备注册响应：code=0 表示成功，同步充电桩列表并启动心跳
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
    // 同步响应：遍历结果，COMPLETED 状态订单在本地结算
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
    // 服务端下发开始充电指令
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
    // 服务端下发停止充电指令
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
    // 服务端返回全局 orderId 用于映射
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
    // 服务端确认结算订单
    else if(type == "device.order.settled") {
        bool settled = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &settled, &error]() {
            settled = m_database->settleOrder(payload.value("orderId").toVariant().toLongLong(), &error);
        }, Qt::BlockingQueuedConnection);
        if(settled) data = {{"settled", true}};
    }
    // 服务端中止订单（充电桩恢复 IDLE）
    else if(type == "device.order.abort") {
        bool aborted = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &aborted, &error]() {
            aborted = m_database->abortOrder(payload.value("orderId").toVariant().toLongLong(), &error);
        }, Qt::BlockingQueuedConnection);
        if(aborted) data = {{"aborted", true}};
    }
    // 断网期间订单支付状态批量更新为 PAID
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
    // 服务端推送新增充电桩，同步本地数据库
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
    // 服务端推送删除充电桩，同步本地数据库
    else if(type == "device.charger.removed") {
        bool removed = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &removed, &error]() {
            removed = m_database->removeChargerByCode(payload.value("code").toString(), &error);
        }, Qt::BlockingQueuedConnection);
        if(removed) {
            const QString code = payload.value("code").toString();
            m_soc.remove(code);
            emit chargerRemoved(code);
            qInfo() << "Charger removed from server:" << code;
            QMetaObject::invokeMethod(m_database, [this, &error]() {
                m_database->removeEmptyStations(&error);
            }, Qt::BlockingQueuedConnection);
        }
        return;
    }
    // 服务端推送更新充电桩，同步本地数据库
    else if(type == "device.charger.updated") {
        bool updated = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &updated, &error]() {
            updated = m_database->updateChargerFromServer(
                payload.value("code").toString(),
                payload.value("type").toString(),
                payload.value("ratedPower").toDouble(),
                payload.value("stationName").toString(),
                &error);
        }, Qt::BlockingQueuedConnection);
        if(updated) {
            const QString code = payload.value("code").toString();
            emit chargerUpdated(code);
            qInfo() << "Charger updated from server:" << code;
        } else {
            qWarning() << "Failed to update charger:" << error;
        }
        return;
    }
    // 服务端推送新增站点
    else if(type == "device.station.added") {
        bool added = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &added, &error]() {
            added = m_database->addStationFromServer(
                payload.value("name").toString(), &error);
        }, Qt::BlockingQueuedConnection);
        if(added) {
            emit stationAdded(payload.value("name").toString());
            qInfo() << "Station added from server:" << payload.value("name").toString();
        }
        return;
    }
    // 服务端推送站点改名
    else if(type == "device.station.updated") {
        bool updated = false;
        QMetaObject::invokeMethod(m_database, [this, payload, &updated, &error]() {
            updated = m_database->updateStationName(
                payload.value("oldName").toString(),
                payload.value("newName").toString(),
                &error);
        }, Qt::BlockingQueuedConnection);
        if(updated) {
            emit stationUpdated(payload.value("oldName").toString(),
                                payload.value("newName").toString());
            qInfo() << "Station updated:" << payload.value("oldName").toString()
                    << "->" << payload.value("newName").toString();
        }
        return;
    }
    // 服务端推送删除站点（级联删除充电桩）
    else if(type == "device.station.removed") {
        QStringList removedChargers;
        QMetaObject::invokeMethod(m_database, [this, payload, &removedChargers, &error]() {
            removedChargers = m_database->removeStationByName(
                payload.value("name").toString(), &error);
        }, Qt::BlockingQueuedConnection);
        const QString stationName = payload.value("name").toString();
        for(const QString &code : removedChargers) {
            m_soc.remove(code);
            emit chargerRemoved(code);
        }
        emit stationRemoved(stationName);
        qInfo() << "Station removed from server:" << stationName
                << "(" << removedChargers.size() << "chargers removed)";
        return;
    }
    else return;
    // 向服务端发送响应
    emit sendMessage(Protocol::response(message, error.isEmpty() ? 0 : 400,
                                         error.isEmpty() ? "ok" : error, data));
}
