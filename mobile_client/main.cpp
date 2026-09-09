// 移动端客户端入口：配置 QML 引擎并加载界面
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include "mobileclient.h"
#include "apptheme.h"

int main(int argc,char *argv[])
{
    // 启用高 DPI 缩放,适配不同屏幕
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc,argv);
    // 设置应用名称和 Material 主题
    QGuiApplication::setApplicationName(QStringLiteral("悦充"));
    QQuickStyle::setStyle(QStringLiteral("Material"));
    // 创建核心对象并注入 QML 上下文
    MobileClient client;
    AppTheme theme;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("mobileClient"),&client);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"),&theme);
    // 加载 QML 主界面
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if(engine.rootObjects().isEmpty())return 1;
    return app.exec();
}
