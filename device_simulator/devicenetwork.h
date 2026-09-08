#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>

class DeviceNetwork : public QObject
{
    Q_OBJECT
public:
    explicit DeviceNetwork(QObject *parent = nullptr);

signals:
    void connected();
    void disconnected();
    void messageReceived(QJsonObject message);
    void connectionError(QString error);

public slots:
    void connectToHost(QString host, quint16 port);
    void disconnectFromHost();
    void sendMessage(QJsonObject message);

private slots:
    void onEncrypted();
    void onReadyRead();
    void onDisconnected();
    void onSslErrors(const QList<QSslError> &errors);
    void onReconnectTimer();

private:
    QSslSocket *m_socket;
    QByteArray m_buffer;
    QTimer *m_reconnectTimer;
    QString m_host;
    quint16 m_port = 0;
    bool m_manualDisconnect = false;
};
