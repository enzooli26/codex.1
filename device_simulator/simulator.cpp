#include "simulator.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QRandomGenerator>
#include <QDebug>
#include <QUuid>

Simulator::Simulator(const QString &codes, QObject *p)
    : QObject(p)
{
    m_codes = codes.split(',', Qt::SkipEmptyParts);
    for (const auto &code : m_codes)
        m_soc.insert(code, 35.0);
    m_heartbeat.setInterval(5000);
    m_telemetry.setInterval(2000);
    connect(&m_heartbeat, &QTimer::timeout, this, &Simulator::heartbeat);
    connect(&m_telemetry, &QTimer::timeout, this, &Simulator::telemetry);
    connect(&m_socket, &QSslSocket::encrypted, this, [this]{
        m_heartbeat.start();
        m_telemetry.start();
        heartbeat();
    });
    connect(&m_socket, &QSslSocket::disconnected, this, &Simulator::reconnect);
}

void Simulator::start(const QString &host, quint16 port)
{
    m_socket.setProperty("host", host);
    m_socket.setProperty("port", port);
    QString error;
    if (!SecureConnect::connectToServer(&m_socket, host, port, &error))
        qWarning() << error;
}

void Simulator::reconnect()
{
    m_heartbeat.stop();
    m_telemetry.stop();
    QTimer::singleShot(3000, this, [this]{
        QString error;
        SecureConnect::connectToServer(&m_socket,
            m_socket.property("host").toString(),
            static_cast<quint16>(m_socket.property("port").toUInt()),
            &error);
    });
}

void Simulator::send(const QString &t, const QJsonObject &p)
{
    if (m_socket.isEncrypted())
        m_socket.write(Protocol::encode(
            Protocol::request(t, p, QUuid::createUuid().toString(QUuid::WithoutBraces))));
}

void Simulator::heartbeat()
{
    for (const auto &code : m_codes)
        send("device.heartbeat", {{"chargerCode", code}, {"status", "IDLE"}});
}

void Simulator::telemetry()
{
    for (const auto &code : m_codes) {
        const double current = 30.0 + QRandomGenerator::global()->bounded(1000) / 100.0;
        const double voltage = 380.0 + QRandomGenerator::global()->bounded(500) / 100.0;
        const double power = voltage * current / 1000.0;
        double &soc = m_soc[code];
        soc = qMin(100.0, soc + 0.05);
        send("device.telemetry", {{"chargerCode", code},
                                  {"voltage", voltage},
                                  {"current", current},
                                  {"power", power},
                                  {"soc", soc}});
    }
}
