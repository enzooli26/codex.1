#pragma once

#include <QWidget>
#include <QSslSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QTimer>
#include "ui_login.h"

class LoginWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

signals:
    void loginSuccess(QSslSocket *socket, qint64 userId,
                      const QString &nickname, double balance);
    void showRegister();

private slots:
    void onConnectServer();
    void onEncrypted();
    void onDisconnected();
    void onReadMessages();
    void onLogin();
    void onSslErrors(const QList<QSslError> &errors);

private:
    void sendRequest(const QString &type, const QJsonObject &payload = {});
    void processMessage(const QJsonObject &message);

    Ui::LoginWindow *ui;
    QSslSocket m_socket;
    QByteArray m_buffer;
};
