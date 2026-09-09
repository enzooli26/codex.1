#include "networkworker.h"
#include "framecodec.h"
#include "secureconnect.h"
#include <QUuid>

//用于处理所有的网络连接
NetworkWorker::NetworkWorker(QObject *parent) : QObject(parent)
{
    m_socket = new QSslSocket(this);
    connect(m_socket, &QSslSocket::readyRead,
            this, &NetworkWorker::onReadyRead);
    connect(m_socket, &QSslSocket::encrypted,
            this, &NetworkWorker::onEncrypted);
    connect(m_socket, &QSslSocket::disconnected,
            this, &NetworkWorker::onDisconnected);
    connect(m_socket,
            QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &NetworkWorker::onSslErrors);
}

// 析构函数：中断连接并释放 socket 资源
NetworkWorker::~NetworkWorker()
{
    if (m_socket) {
        m_socket->abort();
        m_socket->deleteLater();
    }
}

//查询当前是否已建立 TLS 连接
bool NetworkWorker::isEncrypted() const
{
    return m_socket && m_socket->isEncrypted();
}

// 发起 TLS 连接到服务端
void NetworkWorker::connectToServer(const QString &host, quint16 port)
{
    m_buffer.clear();
    m_socket->abort();
    QString error;
    if (!SecureConnect::connectToServer(m_socket, host, port, &error))
        emit connectionError(error);
}

// 断开与服务端的连接
void NetworkWorker::disconnectFromServer()
{
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

// 发送带 UUID 的请求消息（仅在 TLS 连接建立后有效）
void NetworkWorker::sendRequest(const QString &type, const QJsonObject &payload)
{
    if (!m_socket || !m_socket->isEncrypted())
        return;
    const QByteArray frame = Protocol::encode(
        Protocol::request(type, payload,
            QUuid::createUuid().toString(QUuid::WithoutBraces)));
    m_socket->write(frame);
}

// 接收数据，帧解码后分发消息
void NetworkWorker::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    QString err;
    const QList<QJsonObject> messages = Protocol::decode(m_buffer, &err);
    for (const QJsonObject &msg : messages)
        emit messageReceived(msg);
}

// TLS 握手成功，发出连接信号
void NetworkWorker::onEncrypted()
{
    emit connected();
}

// 连接断开，发出断开信号
void NetworkWorker::onDisconnected()
{
    emit disconnected();
}

// 忽略 SSL 错误并发出错误信号
void NetworkWorker::onSslErrors(const QList<QSslError> &errors)
{
    if (!errors.isEmpty())
        emit sslErrorsOccurred(errors.first().errorString());
    m_socket->ignoreSslErrors();
}
