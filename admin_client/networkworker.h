#pragma once

#include <QObject>
#include <QSslSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QSslError>

class NetworkWorker : public QObject
{
    Q_OBJECT
public:
    explicit NetworkWorker(QObject *parent = nullptr);
    ~NetworkWorker();

    bool isEncrypted() const;

public slots:
    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    void sendRequest(const QString &type, const QJsonObject &payload = QJsonObject());

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void messageReceived(const QJsonObject &msg);
    void sslErrorsOccurred(const QString &errorString);

private slots:
    void onReadyRead();
    void onEncrypted();
    void onDisconnected();
    void onSslErrors(const QList<QSslError> &errors);

private:
    QSslSocket *m_socket;
    QByteArray   m_buffer;
};
