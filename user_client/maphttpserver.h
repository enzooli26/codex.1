#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>

class MapHttpServer : public QObject
{
    Q_OBJECT
public:
    explicit MapHttpServer(QObject *parent = nullptr);
    bool isListening() const { return m_server.isListening(); }
    QUrl url() const;
private slots:
    void onNewConnection();
private:
    void handleRequest(QTcpSocket *socket);
    QTcpServer m_server;
    QByteArray m_htmlBytes;
};
