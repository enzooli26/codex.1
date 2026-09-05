#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include "mobileclient.h"

int main(int argc,char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc,argv);
    QGuiApplication::setApplicationName(QStringLiteral("悦充"));
    QQuickStyle::setStyle(QStringLiteral("Material"));
    MobileClient client;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("mobileClient"),&client);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if(engine.rootObjects().isEmpty())return 1;
    return app.exec();
}
