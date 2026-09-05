#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include "serverapp.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("ev_server"));
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({{"c", "config"}, "Configuration file", "path", "config/app.ini"});
    parser.process(app);

    const QFileInfo configInfo(parser.value("config"));
    QSettings settings(configInfo.absoluteFilePath(), QSettings::IniFormat);
    const QDir configDir=configInfo.absoluteDir();
    auto resolve=[&configDir](const QString &value){return QFileInfo(value).isAbsolute()?value:configDir.absoluteFilePath(value);};
    const quint16 port = static_cast<quint16>(settings.value("server/port", 9527).toUInt());
    const QString dbPath = settings.value("database/path", "data/charging.db").toString();
    const QString certificatePath = resolve(settings.value("tls/certificate", "server-cert.pem").toString());
    const QString privateKeyPath = resolve(settings.value("tls/private_key", "server-key.pem").toString());
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    ServerApp server;
    if (!server.start(port, dbPath, certificatePath, privateKeyPath)) return 1;
    qInfo() << "EV TLS server listening on" << port << "database" << dbPath;
    return app.exec();
}
