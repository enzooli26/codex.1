#include "maphttpserver.h"
#include <QUrl>
#include <QFile>

MapHttpServer::MapHttpServer(QObject *parent)
    : QObject(parent)
{
    QFile f(QStringLiteral(":/tencent_map.html"));
    if (f.open(QIODevice::ReadOnly)) {
        m_htmlBytes = f.readAll();
    } else {
        m_htmlBytes = "<html><body><h1>tencent_map.html not found</h1></body></html>";
    }
    connect(&m_server, &QTcpServer::newConnection, this, &MapHttpServer::onNewConnection);
    if (m_server.listen(QHostAddress::LocalHost, 0)) {
        qDebug() << "[MapHTTP] listening on" << url().toString();
    } else {
        qWarning() << "[MapHTTP] listen failed:" << m_server.errorString();
    }
}

QUrl MapHttpServer::url() const
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/tencent_map.html").arg(m_server.serverPort()));
}

void MapHttpServer::onNewConnection()
{
    while (m_server.hasPendingConnections()) {
        QTcpSocket *socket = m_server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]{ handleRequest(socket); });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    }
}

void MapHttpServer::handleRequest(QTcpSocket *socket)
{
    QString method, path;
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (method.isEmpty()) {
            QList<QByteArray> parts = line.split(' ');
            if (parts.size() >= 2) {
                method = QString::fromLatin1(parts[0]);
                path   = QString::fromLatin1(parts[1]);
            }
        }
    }
    if (method == "GET" && (path.contains("tencent_map") || path == "/")) {
        QByteArray header = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html; charset=utf-8\r\n"
                            "Content-Length: " + QByteArray::number(m_htmlBytes.size()) + "\r\n"
                            "Connection: close\r\n"
                            "Cache-Control: no-cache\r\n\r\n";
        socket->write(header);
        socket->write(m_htmlBytes);
    } else {
        QByteArray resp = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        socket->write(resp);
    }
    socket->flush();
    disconnect(socket, &QTcpSocket::readyRead, this, nullptr);
    socket->disconnectFromHost();
}
