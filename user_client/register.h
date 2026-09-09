#pragma once

#include <QWidget>
#include <QSslSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QTimer>
#include "ui_register.h"

class RegisterWindow : public QWidget
{
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

signals:
    void loginSuccess(QSslSocket *socket, qint64 userId, const QString &nickname, double balance);
    void backToLogin();

private slots:
    void onConnectServer();
    void onEncrypted();
    void onReadMessages();
    void onRegister();
    void onSslErrors(const QList<QSslError> &errors);

private:
    void sendRequest(const QString &type, const QJsonObject &payload = {});
    void processMessage(const QJsonObject &message);

    Ui::RegisterWindow *ui;
    QSslSocket m_socket;
    QByteArray m_buffer;
    QString m_lastPhone;
    QString m_lastPassword;
};
