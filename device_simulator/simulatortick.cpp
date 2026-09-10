#include "simulatortick.h"

// 创建三个定时器：心跳 5s、遥测 2s、超时 10s（单次触发）
SimulatorTick::SimulatorTick(QObject *parent)
    : QObject(parent)
{
    m_heartbeatTimer = new QTimer(this);
    m_telemetryTimer = new QTimer(this);
    m_timeoutTimer = new QTimer(this);
    //心跳定时
    m_heartbeatTimer->setInterval(5000);
    //遥测，持续计费在此实现
    m_telemetryTimer->setInterval(2000);
    //超时检测
    m_timeoutTimer->setInterval(10000);
    m_timeoutTimer->setSingleShot(true);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &SimulatorTick::heartbeatTick);
    connect(m_telemetryTimer, &QTimer::timeout, this, &SimulatorTick::telemetryTick);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SimulatorTick::heartbeatTimeout);
}

// 启动所有定时器
void SimulatorTick::start()
{
    m_heartbeatTimer->start();
    m_telemetryTimer->start();
    m_timeoutTimer->start();
}

// 停止所有定时器
void SimulatorTick::stop()
{
    m_heartbeatTimer->stop();
    m_telemetryTimer->stop();
    m_timeoutTimer->stop();
}

//断联期间保证计费不间断
void SimulatorTick::stopHeartbeat()
{
    m_heartbeatTimer->stop();
    m_timeoutTimer->stop();
}

// 收到心跳响应后重置超时计时器
void SimulatorTick::resetHeartbeatTimer()
{
    m_timeoutTimer->start();
}

// 重置遥测定时器
void SimulatorTick::resetTelemetryTimer()
{
    m_telemetryTimer->start();
}
