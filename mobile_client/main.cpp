#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include "mobileclient.h"
#include "apptheme.h"

int main(int argc,char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc,argv);
    QGuiApplication::setApplicationName(QStringLiteral("悦充"));
    QQuickStyle::setStyle(QStringLiteral("Material"));
    MobileClient client;
    AppTheme theme;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("mobileClient"),&client);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"),&theme);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if(engine.rootObjects().isEmpty())return 1;
    return app.exec();
}
