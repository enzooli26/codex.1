#include "simulator.h"
#include "framecodec.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>
#include <QSet>
#include <QUuid>

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
    QHash<QString, QJsonObject> localMap;
    for(const auto &v : localChargers) {
        const QJsonObject c = v.toObject();
        const QString code = c.value("code").toString();
        localCodes.insert(code);
        localMap[code] = c;
    }

    QStringList serverCodeList;
    QHash<QString, QJsonObject> serverMap;
    for(const auto &v : serverChargers) {
        const QJsonObject c = v.toObject();
        const QString code = c.value("code").toString();
        if(code.isEmpty()) continue;
        serverCodeList.append(code);
        serverMap[code] = c;
    }

    for(const auto &v : serverChargers) {
        const QJsonObject c = v.toObject();
        const QString code = c.value("code").toString();
        if(code.isEmpty()) continue;
        if(localCodes.contains(code)) {
            bool updated = false;
            QMetaObject::invokeMethod(m_database, [this, code, c, &updated, &err]() {
                updated = m_database->updateChargerFromServer(
                    code,
                    c.value("type").toString("FAST"),
                    c.value("ratedPower").toDouble(120),
                    c.value("stationName").toString("未分配站点"),
                    &err);
            }, Qt::BlockingQueuedConnection);
            if(updated) {
                emit chargerUpdated(code);
                qInfo() << "Synced updated charger from server:" << code;
            }
        } else {
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

    QStringList removed;
    QMetaObject::invokeMethod(m_database, [this, serverCodeList, &removed, &err]() {
        removed = m_database->removeChargersNotIn(serverCodeList, &err);
    }, Qt::BlockingQueuedConnection);
    for(const QString &code : removed) {
        m_soc.remove(code);
        emit chargerRemoved(code);
        qInfo() << "Removed local charger not on server:" << code;
    }

    QStringList removedStations;
    QMetaObject::invokeMethod(m_database, [this, &removedStations, &err]() {
        removedStations = m_database->removeEmptyStations(&err);
    }, Qt::BlockingQueuedConnection);
    for(const QString &name : removedStations) {
        emit stationRemoved(name);
        qInfo() << "Removed empty station:" << name;
    }
}
