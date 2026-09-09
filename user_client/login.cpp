#include "login.h"
#include "framecodec.h"
#include "secureconnect.h"
#include "passwordutils.h"
#include <QMessageBox>
#include <QUuid>
#include <QDebug>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(ui->registerBtn, &QPushButton::clicked, this, &LoginWindow::showRegister);
    connect(&m_socket, &QSslSocket::encrypted, this, &LoginWindow::onEncrypted);
    connect(&m_socket, &QSslSocket::disconnected, this, &LoginWindow::onDisconnected);
    connect(&m_socket, &QSslSocket::readyRead, this, &LoginWindow::onReadMessages);
    connect(&m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &LoginWindow::onSslErrors);
    ui->loginButton->setEnabled(false);
    onConnectServer();
}

LoginWindow::~LoginWindow() { delete ui; }

void LoginWindow::onConnectServer()
{
    m_socket.abort();
    QString error;
    if (!SecureConnect::connectToServer(&m_socket, "127.0.0.1", 9527, &error)) {
        ui->hintLabel->setText("连接失败：" + error);
        return;
    }
    ui->hintLabel->setText("正在连接服务器…");
}

void LoginWindow::onEncrypted()
{
    ui->hintLabel->setText("已连接，请输入账号密码登录");
    ui->loginButton->setEnabled(true);
}

void LoginWindow::onDisconnected()
{
    ui->loginButton->setEnabled(false);
    ui->hintLabel->setText("连接已断开，正在重连…");
    QTimer::singleShot(2000, this, &LoginWindow::onConnectServer);
}

void LoginWindow::onSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
    ui->hintLabel->setText("TLS 错误：" + m_socket.errorString());
}

void LoginWindow::onLogin()
{
    const QString phone = ui->phoneEdit->text().trimmed();
    const QString password = ui->passwordEdit->text();
    if (!PasswordUtils::validPhone(phone)) {
        QMessageBox::warning(this, "输入错误", "请输入合法的 11 位手机号");
        return;
    }
    if (password.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "密码不能为空");
        return;
    }
    ui->loginButton->setEnabled(false);
    ui->hintLabel->setText("登录中…");
    m_lastPhone = phone;
    m_lastPassword = password;
    sendRequest("auth.user", {{"phone", phone}, {"password", password}});
}

void LoginWindow::sendRequest(const QString &type, const QJsonObject &payload)
{
    m_socket.write(Protocol::encode(
        Protocol::request(type, payload, QUuid::createUuid().toString(QUuid::WithoutBraces))));
}

void LoginWindow::onReadMessages()
{
    m_buffer.append(m_socket.readAll());
    QString error;
    for (const auto &m : Protocol::decode(m_buffer, &error))
        processMessage(m);
    if (!error.isEmpty())
        QMessageBox::warning(this, "协议错误", error);
}

void LoginWindow::processMessage(const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    if (type == "auth.user.result") {
        ui->loginButton->setEnabled(true);
        if (message.value("code").toInt() != 0) {
            ui->hintLabel->setText("");
            QMessageBox::warning(this, "登录失败", message.value("message").toString());
            return;
        }
        const QJsonObject data = message.value("data").toObject();
        const qint64 userId = data.value("id").toVariant().toLongLong();
        const QString nickname = data.value("nickname").toString();
        const double balance = data.value("balance").toDouble();
        QObject::disconnect(&m_socket, &QSslSocket::readyRead, this, &LoginWindow::onReadMessages);
        QObject::disconnect(&m_socket, &QSslSocket::disconnected, this, &LoginWindow::onDisconnected);
        emit loginSuccess(&m_socket, userId, nickname, balance, m_lastPhone, m_lastPassword);
    }
}
