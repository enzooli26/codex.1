#include "simulatortick.h"

SimulatorTick::SimulatorTick(QObject *parent)
    : QObject(parent)
{
    m_heartbeatTimer = new QTimer(this);
    m_telemetryTimer = new QTimer(this);
    m_timeoutTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(5000);
    m_telemetryTimer->setInterval(2000);
    m_timeoutTimer->setInterval(10000);
    m_timeoutTimer->setSingleShot(true);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &SimulatorTick::heartbeatTick);
    connect(m_telemetryTimer, &QTimer::timeout, this, &SimulatorTick::telemetryTick);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SimulatorTick::heartbeatTimeout);
}

void SimulatorTick::start()
{
    m_heartbeatTimer->start();
    m_telemetryTimer->start();
    m_timeoutTimer->start();
}

void SimulatorTick::stop()
{
    m_heartbeatTimer->stop();
    m_telemetryTimer->stop();
    m_timeoutTimer->stop();
}

void SimulatorTick::stopHeartbeat()
{
    m_heartbeatTimer->stop();
    m_timeoutTimer->stop();
}

void SimulatorTick::resetHeartbeatTimer()
{
    m_timeoutTimer->start();
}

void SimulatorTick::resetTelemetryTimer()
{
    m_telemetryTimer->start();
}
