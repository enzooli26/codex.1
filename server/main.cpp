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

    QSettings settings(parser.value("config"), QSettings::IniFormat);
    const quint16 port = static_cast<quint16>(settings.value("server/port", 9527).toUInt());
    const QString dbPath = settings.value("database/path", "data/charging.db").toString();
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    ServerApp server;
    if (!server.start(port, dbPath)) return 1;
    qInfo() << "EV server listening on" << port << "database" << dbPath;
    return app.exec();
}

