#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include "simulator.h"

int main(int argc,char *argv[])
{
    QCoreApplication app(argc,argv);QCoreApplication::setApplicationName("ev_device_simulator");
    QCommandLineParser p;p.addHelpOption();
    p.addOption({"host","Central server host","host","127.0.0.1"});
    p.addOption({"port","Central server port","port","9527"});
    p.addOption({"code","Comma-separated charger codes","codes","DL-SW-001,DL-SW-002,DL-GX-001"});
    p.addOption({"database","Local SQLite database path","path","data/charger-edge.db"});
    p.addOption({"token","Shared charger credential","token","course-device-token"});p.process(app);
    QStringList codes=p.value("code").split(',',Qt::SkipEmptyParts);for(QString &code:codes)code=code.trimmed();codes.removeDuplicates();
    if(codes.isEmpty()){qCritical()<<"at least one charger code is required";return 1;}
    Simulator simulator(codes,QDir::cleanPath(QDir::current().absoluteFilePath(p.value("database"))),p.value("token"));QString error;
    if(!simulator.initialize(&error)){qCritical()<<"cannot open charger database"<<error;return 1;}
    simulator.start(p.value("host"),static_cast<quint16>(p.value("port").toUInt()));return app.exec();
}
