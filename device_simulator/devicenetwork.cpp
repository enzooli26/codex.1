#include "devicenetwork.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QDebug>

// 创建 SSL socket 和 3s 重连定时器，绑定信号槽
DeviceNetwork::DeviceNetwork(QObject *parent)
    : QObject(parent)
{
    m_socket = new QSslSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_socket, &QSslSocket::encrypted, this, &DeviceNetwork::onEncrypted);
    connect(m_socket, &QSslSocket::readyRead, this, &DeviceNetwork::onReadyRead);
    //如果服务端断开Qssl会自动处理，调用onDisconnected
    connect(m_socket, &QSslSocket::disconnected, this, &DeviceNetwork::onDisconnected);
    connect(m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &DeviceNetwork::onSslErrors);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DeviceNetwork::onReconnectTimer);
}

// 发起 TLS 连接到服务端
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

// 断开连接（标记手动断开，不自动重连）
void DeviceNetwork::disconnectFromHost()
{
    m_manualDisconnect = true;
    m_reconnectTimer->stop();
    if(m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

// 发送 JSON 消息（经 Protocol 编码为二进制帧）
void DeviceNetwork::sendMessage(QJsonObject message)
{
    if(m_socket->isEncrypted())
        m_socket->write(Protocol::encode(message));
}

// TLS 握手成功，发出连接信号
void DeviceNetwork::onEncrypted()
{
    emit connected();
}

// 接收数据，帧解码后分发消息
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

// 连接断开：非手动断开时启动 3s 重连定时器
void DeviceNetwork::onDisconnected()
{
    emit disconnected();
    if(!m_manualDisconnect)
        m_reconnectTimer->start();
}

// 忽略 SSL 错误（开发环境证书）
void DeviceNetwork::onSslErrors(const QList<QSslError> &errors)
{
    qWarning() << "SSL errors:" << errors;
    m_socket->ignoreSslErrors();
}

// 自动重连逻辑
void DeviceNetwork::onReconnectTimer()
{
    if(m_manualDisconnect) return;
    QString error;
    if(!SecureConnect::connectToServer(m_socket, m_host, m_port, &error)) {
        qWarning() << "reconnect error:" << error;
        m_reconnectTimer->start();
    }
}
