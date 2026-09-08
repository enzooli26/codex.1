#include "devicenetwork.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QDebug>

DeviceNetwork::DeviceNetwork(QObject *parent)
    : QObject(parent)
{
    m_socket = new QSslSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_socket, &QSslSocket::encrypted, this, &DeviceNetwork::onEncrypted);
    connect(m_socket, &QSslSocket::readyRead, this, &DeviceNetwork::onReadyRead);
    connect(m_socket, &QSslSocket::disconnected, this, &DeviceNetwork::onDisconnected);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &DeviceNetwork::onSslErrors);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DeviceNetwork::onReconnectTimer);
}

void DeviceNetwork::connectToHost(QString host, quint16 port)
{
    m_host = host;
    m_port = port;
    m_manualDisconnect = false;
    QString error;
    if(!SecureConnect::connectToServer(m_socket, host, port, &error)) {
        qWarning() << "Network connect error:" << error;
        emit connectionError(error);
    }
}

void DeviceNetwork::disconnectFromHost()
{
    m_manualDisconnect = true;
    m_reconnectTimer->stop();
    if(m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void DeviceNetwork::sendMessage(QJsonObject message)
{
    if(m_socket->isEncrypted())
        m_socket->write(Protocol::encode(message));
}

void DeviceNetwork::onEncrypted()
{
    emit connected();
}

void DeviceNetwork::onReadyRead()
{
    m_buffer += m_socket->readAll();
    QString error;
    for(const QJsonObject &message : Protocol::decode(m_buffer, &error))
        emit messageReceived(message);
    if(!error.isEmpty()) {
        qWarning() << "protocol error" << error;
        m_socket->disconnectFromHost();
    }
}

void DeviceNetwork::onDisconnected()
{
    emit disconnected();
    if(!m_manualDisconnect)
        m_reconnectTimer->start();
}

void DeviceNetwork::onSslErrors(const QList<QSslError> &errors)
{
    qWarning() << "SSL errors:" << errors;
    m_socket->ignoreSslErrors();
}

void DeviceNetwork::onReconnectTimer()
{
    if(m_manualDisconnect) return;
    QString error;
    if(!SecureConnect::connectToServer(m_socket, m_host, m_port, &error)) {
        qWarning() << "reconnect error:" << error;
        m_reconnectTimer->start();
    }
}
