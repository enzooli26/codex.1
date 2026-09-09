#include <QApplication>
#include <QtWebEngine/QtWebEngine>
#include "login.h"
#include "register.h"
#include "userwindow.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QtWebEngine::initialize();
    QApplication app(argc, argv);

    UserWindow *userWindow = nullptr;
    RegisterWindow *registerWin = nullptr;
    LoginWindow login;

    auto enterMain = [&](QSslSocket *socket, qint64 userId, const QString &nickname, double balance,
                         const QString &phone, const QString &password) {
        if (!userWindow) {
            userWindow = new UserWindow;
            QObject::connect(userWindow, &UserWindow::reconnectRequested, [&]() {
                userWindow->hide();
                login.show();
            });
        }
        userWindow->setConnection(socket, userId, nickname, balance, phone, password);
        userWindow->show();
        login.hide();
        if (registerWin) registerWin->hide();
    };

    QObject::connect(&login, &LoginWindow::loginSuccess, enterMain);

    QObject::connect(&login, &LoginWindow::showRegister, [&]() {
        if (!registerWin) {
            registerWin = new RegisterWindow;
            QObject::connect(registerWin, &RegisterWindow::backToLogin, [&]() {
                registerWin->hide();
                login.show();
            });
            QObject::connect(registerWin, &RegisterWindow::loginSuccess, enterMain);
        }
        registerWin->show();
        login.hide();
    });

    login.show();
    return app.exec();
}
