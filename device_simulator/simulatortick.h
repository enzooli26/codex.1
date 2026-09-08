#pragma once

#include <QObject>
#include <QTimer>

class SimulatorTick : public QObject
{
    Q_OBJECT
public:
    explicit SimulatorTick(QObject *parent = nullptr);

    void start();
    void stop();
    void resetHeartbeatTimer();
    void resetTelemetryTimer();

signals:
    void heartbeatTick();
    void telemetryTick();
    void heartbeatTimeout();

private:
    QTimer m_heartbeatTimer;
    QTimer m_telemetryTimer;
    QTimer m_timeoutTimer;
};
