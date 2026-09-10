#include "simulator.h"
#include "devicenetwork.h"
#include "simulatortick.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>

// 初始化充电桩默认 SOC 为 35%
Simulator::Simulator(const QStringList &codes, const QString &databasePath,
                     const QString &token, QObject *parent)
    : QObject(parent), m_codes(codes), m_databasePath(databasePath), m_token(token)
{
    for(const QString &code : m_codes) m_soc[code] = 35.0;
}

// 建立 Simulator与DeviceNetwork 信号连接
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

// 保存数据库指针
void Simulator::setDatabase(EdgeDatabase *database)
{
    m_database = database;
}

// 建立 Simulator ↔ SimulatorTick 信号连接
void Simulator::setTick(SimulatorTick *tick)
{
    m_tick = tick;
    connect(tick, &SimulatorTick::heartbeatTick, this, &Simulator::onHeartbeatTick);
    connect(tick, &SimulatorTick::telemetryTick, this, &Simulator::onTelemetryTick);
    connect(tick, &SimulatorTick::heartbeatTimeout, this, &Simulator::onHeartbeatTimeout);
}

// 在数据库线程中打开 SQLite 并创建表结构
bool Simulator::initialize(QString *error)
{
    bool ok = false;
    QMetaObject::invokeMethod(m_database, [this, &ok, error]() {
        ok = m_database->open(m_databasePath, m_codes, error);
    }, Qt::BlockingQueuedConnection);
    return ok;
}

// 记录主机信息并触发网络连接
void Simulator::start(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;
    emit connectNetwork(host, port);
}

// 手动断开：停止定时器，清注册状态，触发断开信号
void Simulator::disconnectFromServer()
{
    m_manualDisconnect = true;
    m_registered = false;
    m_heartbeatFailures = 0;
    if(m_tick) QMetaObject::invokeMethod(m_tick, "stopHeartbeat", Qt::QueuedConnection);
    emit disconnectNetwork();
    qInfo() << "simulator manually disconnected; local charging continues";
    emit connectionChanged(false);
    m_disconnected = true;
    emit disconnectedStateChanged(m_disconnected);
    checkDisconnection();
}

// 手动重连：重新发起网络连接
void Simulator::connectToServer()
{
    m_manualDisconnect = false;
    emit connectNetwork(m_host, m_port);
}

// 查询所有站点（含充电桩统计）
QJsonArray Simulator::stations(QString *error)
{
    QJsonArray result;
    QMetaObject::invokeMethod(m_database, [this, &result, error]() {
        result = m_database->stations(error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

// 查询指定站点下的充电桩
QJsonArray Simulator::chargersByStation(int stationId, QString *error)
{
    QJsonArray result;
    QMetaObject::invokeMethod(m_database, [this, stationId, &result, error]() {
        result = m_database->chargersByStation(stationId, error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

// 查询充电桩的当前活跃订单
QJsonObject Simulator::activeOrderForCharger(const QString &code, QString *error)
{
    QJsonObject result;
    QMetaObject::invokeMethod(m_database, [this, &code, &result, error]() {
        result = m_database->activeOrderForCharger(code, error);
    }, Qt::BlockingQueuedConnection);
    return result;
}

// 查询充电桩状态（IDLE/CHARGING/FAULT）
QString Simulator::chargerStatus(const QString &code, QString *error)
{
    QString result;
    QMetaObject::invokeMethod(m_database, [this, code, &result, error]() {
        result = m_database->chargerStatus(code, error);
    }, Qt::BlockingQueuedConnection);
    return result;
}
