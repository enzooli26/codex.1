#include "register.h"
#include "framecodec.h"
#include "secureconnect.h"
#include "passwordutils.h"
#include <QMessageBox>
#include <QUuid>
#include <QDebug>

RegisterWindow::RegisterWindow(QWidget *parent)
    : QWidget(parent), ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);
    connect(ui->registerButton, &QPushButton::clicked, this, &RegisterWindow::onRegister);
    connect(ui->backBtn, &QPushButton::clicked, this, &RegisterWindow::backToLogin);
    connect(&m_socket, &QSslSocket::encrypted, this, &RegisterWindow::onEncrypted);
    connect(&m_socket, &QSslSocket::readyRead, this, &RegisterWindow::onReadMessages);
    connect(&m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &RegisterWindow::onSslErrors);
    onConnectServer();
}

RegisterWindow::~RegisterWindow() { delete ui; }

void RegisterWindow::onConnectServer()
{
    m_socket.abort();
    QString error;
    if (!SecureConnect::connectToServer(&m_socket, "127.0.0.1", 9527, &error)) {
        qDebug() << "Register connect failed:" << error;
        QTimer::singleShot(2000, this, &RegisterWindow::onConnectServer);
    }
}

void RegisterWindow::onEncrypted()
{
    ui->registerButton->setEnabled(true);
}

void RegisterWindow::onSslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
}

void RegisterWindow::onRegister()
{
    const QString phone = ui->phoneEdit->text().trimmed();
    const QString password = ui->passwordEdit->text();
    const QString confirm = ui->confirmEdit->text();
    if (!PasswordUtils::validPhone(phone)) {
        QMessageBox::warning(this, "输入错误", "请输入合法的 11 位手机号");
        return;
    }
    if (!PasswordUtils::validPassword(password)) {
        QMessageBox::warning(this, "输入错误", "密码长度须为 6～64 位");
        return;
    }
    if (password != confirm) {
        QMessageBox::warning(this, "输入错误", "两次密码输入不一致");
        return;
    }
    m_lastPhone = phone;
    m_lastPassword = password;
    ui->registerButton->setEnabled(false);
    sendRequest("auth.user.register", {{"phone", phone}, {"password", password}, {"confirmPassword", confirm}});
}

void RegisterWindow::sendRequest(const QString &type, const QJsonObject &payload)
{
    m_socket.write(Protocol::encode(
        Protocol::request(type, payload, QUuid::createUuid().toString(QUuid::WithoutBraces))));
}

void RegisterWindow::onReadMessages()
{
    m_buffer.append(m_socket.readAll());
    QString error;
    for (const auto &m : Protocol::decode(m_buffer, &error))
        processMessage(m);
    if (!error.isEmpty())
        QMessageBox::warning(this, "协议错误", error);
}

void RegisterWindow::processMessage(const QJsonObject &message)
{
    const QString type = message.value("type").toString();
    if (type == "auth.user.register.result") {
        if (message.value("code").toInt() != 0) {
            ui->registerButton->setEnabled(true);
            QMessageBox::warning(this, "注册失败", message.value("message").toString());
            return;
        }
        // 注册成功，自动登录
        sendRequest("auth.user", {{"phone", m_lastPhone}, {"password", m_lastPassword}});
        return;
    }
    if (type == "auth.user.result") {
        ui->registerButton->setEnabled(true);
        if (message.value("code").toInt() != 0) {
            QMessageBox::warning(this, "自动登录失败", message.value("message").toString());
            return;
        }
        const QJsonObject data = message.value("data").toObject();
        const qint64 userId = data.value("id").toVariant().toLongLong();
        const QString nickname = data.value("nickname").toString();
        const double balance = data.value("balance").toDouble();
        QObject::disconnect(&m_socket, &QSslSocket::readyRead, this, &RegisterWindow::onReadMessages);
        QMessageBox::information(this, "成功", "注册成功，已自动登录");
        emit loginSuccess(&m_socket, userId, nickname, balance);
    }
}
