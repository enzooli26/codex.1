#include "simulator.h"
#include "devicenetwork.h"
#include "simulatortick.h"
#include <QDebug>
#include <QJsonArray>
#include <QMetaObject>

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
    if(m_tick) QMetaObject::invokeMethod(m_tick, "stopHeartbeat", Qt::QueuedConnection);
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
