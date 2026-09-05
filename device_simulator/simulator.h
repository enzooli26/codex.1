#pragma once
#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QStringList>
#include <QHash>

class Simulator : public QObject
{
    Q_OBJECT
public:
    explicit Simulator(const QString &codes, QObject *parent = nullptr);
    void start(const QString &host, quint16 port);

private slots:
    void heartbeat();
    void telemetry();
    void reconnect();

private:
    void send(const QString &t, const QJsonObject &p);
    QStringList m_codes;
    QSslSocket m_socket;
    QTimer m_heartbeat;
    QTimer m_telemetry;
    QHash<QString, double> m_soc;
};
