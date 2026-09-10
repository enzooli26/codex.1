#include "simulator.h"
#include "framecodec.h"
#include "simulatortick.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>
#include <QRandomGenerator>
#include <QUuid>

// 心跳定时回调：收集所有充电桩活跃订单状态，发送心跳包,向服务器更新数据
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
    if(m_tick) QMetaObject::invokeMethod(m_tick, "resetHeartbeatTimer", Qt::QueuedConnection);
    m_heartbeatFailures = 0;
}

//计费和模拟信号在此实现
// 遥测定时回调：模拟电压/电流/功率/SOC，更新订单能耗
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
        // 模拟电压 380~385V 随机波动
        const double voltage = 380.0 + QRandomGenerator::global()->bounded(500) / 100.0;
        double ratedPower = 0;
        QMetaObject::invokeMethod(m_database, [this, code, &ratedPower, &error]() {
            ratedPower = m_database->chargerRatedPower(code, &error);
        }, Qt::BlockingQueuedConnection);
        // 按额定功率 85%~100% 随机波动
        const double power = ratedPower * (0.85 + QRandomGenerator::global()->bounded(151) / 1000.0);
        // 根据功率和电压计算电流
        const double current = power * 1000.0 / voltage;
        // 每次采样 SOC 增加 0.05%，上限 100%
        m_soc[code] = qMin(100.0, m_soc.value(code, 35.0) + 0.05);
        // 更新订单能量/时长/金额，判断是否达标完成
        QJsonObject order;
        QMetaObject::invokeMethod(m_database, [this, code, voltage, current, power, &order, &error]() {
            order = m_database->tick(code, voltage, current, power, m_soc.value(code), &error);
        }, Qt::BlockingQueuedConnection);
        if(!error.isEmpty()) { qWarning() << error; continue; }
        if(!order.isEmpty()) emit orderChanged(code, order);
        // 向服务端上报遥测数据
        if(m_registered) {
            QJsonObject telemetryPayload = {
                {"chargerCode", code}, {"voltage", voltage},
                {"current", current}, {"power", power}, {"soc", m_soc.value(code)}
            };
            sendRequest("device.telemetry", telemetryPayload);
        }
        // 检查订单是否已完成（ENERGY/AMOUNT/TIME 三种模式）
        if(order.value("status").toString() == "SYNC_PENDING") completed = true;
    }
    // 有订单完成时触发同步
    if(completed && m_registered) sendSync();
}
