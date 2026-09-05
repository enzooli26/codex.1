#include <QCoreApplication>
#include <QCommandLineParser>
#include "simulator.h"
int main(int argc,char*argv[]){QCoreApplication app(argc,argv);QCommandLineParser p;p.addHelpOption();p.addOption({"host","Server host","host","127.0.0.1"});p.addOption({"port","Server port","port","9527"});p.addOption({"code","Charger code","code","DL-SW-001"});p.process(app);Simulator s(p.value("code"));s.start(p.value("host"),static_cast<quint16>(p.value("port").toUInt()));return app.exec();}

