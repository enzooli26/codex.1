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

void NetworkWorker::connectToServer(const QString &host, quint16 port)
{
    m_buffer.clear();
    m_socket->abort();
    QString error;
    if (!SecureConnect::connectToServer(m_socket, host, port, &error))
        emit connectionError(error);
}

void NetworkWorker::disconnectFromServer()
{
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();
}

void NetworkWorker::sendRequest(const QString &type, const QJsonObject &payload)
{
    if (!m_socket || !m_socket->isEncrypted())
        return;
    const QByteArray frame = Protocol::encode(
        Protocol::request(type, payload,
            QUuid::createUuid().toString(QUuid::WithoutBraces)));
    m_socket->write(frame);
}

void NetworkWorker::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    QString err;
    const QList<QJsonObject> messages = Protocol::decode(m_buffer, &err);
    for (const QJsonObject &msg : messages)
        emit messageReceived(msg);
}

void NetworkWorker::onEncrypted()
{
    emit connected();
}

void NetworkWorker::onDisconnected()
{
    emit disconnected();
}

void NetworkWorker::onSslErrors(const QList<QSslError> &errors)
{
    if (!errors.isEmpty())
        emit sslErrorsOccurred(errors.first().errorString());
    m_socket->ignoreSslErrors();
}
