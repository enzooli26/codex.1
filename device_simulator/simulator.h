#pragma once
#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QJsonObject>
class Simulator:public QObject{Q_OBJECT public:explicit Simulator(const QString&code,QObject*p=nullptr);void start(const QString&,quint16);private slots:void heartbeat();void telemetry();void reconnect();private:void send(const QString&,const QJsonObject&);QString m_code;QSslSocket m_socket;QTimer m_heartbeat;QTimer m_telemetry;double m_soc=35.0;};
