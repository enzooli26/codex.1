#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QThread>
#include "devicenetwork.h"
#include "simulator.h"
#include "simulatortick.h"
#include "simwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("ev_device_simulator");

    // 解析命令行参数：host/port/codes/database/token
    QCommandLineParser p;
    p.addHelpOption();
    p.addOption({"host", "Central server host", "host", "127.0.0.1"});
    p.addOption({"port", "Central server port", "port", "9527"});
    p.addOption({"code", "Comma-separated charger codes", "codes", "DL-SW-001,DL-SW-002,DL-GX-001"});
    p.addOption({"database", "Local SQLite database path", "path", "data/charger-edge.db"});
    p.addOption({"token", "Shared charger credential", "token", "course-device-token"});
    p.process(app);

    // 分割、去重、去空，至少需要一个充电桩编号
    QStringList codes = p.value("code").split(',', Qt::SkipEmptyParts);
    for(QString &code : codes) code = code.trimmed();
    codes.removeDuplicates();
    if(codes.isEmpty()) { qCritical() << "at least one charger code is required"; return 1; }

    const QString dbPath = QDir::cleanPath(QDir::current().absoluteFilePath(p.value("database")));
    const QString token = p.value("token");

    //针对不同的功能启动不同的线程
    // ===== 数据库线程 =====
    QThread dbThread;
    dbThread.setObjectName("DatabaseThread");
    EdgeDatabase *database = new EdgeDatabase(nullptr);
    database->moveToThread(&dbThread);
    dbThread.start();

    // ===== 业务线程 =====
    QThread bizThread;
    bizThread.setObjectName("BusinessThread");
    Simulator *simulator = new Simulator(codes, dbPath, token, nullptr);
    simulator->setDatabase(database);
    simulator->moveToThread(&bizThread);
    bizThread.start();

    // ===== 网络线程 =====
    QThread netThread;
    netThread.setObjectName("NetworkThread");
    DeviceNetwork *network = new DeviceNetwork(nullptr);
    network->moveToThread(&netThread);
    netThread.start();

    // ===== 定时任务线程 =====
    QThread tickThread;
    tickThread.setObjectName("TickThread");
    SimulatorTick *tick = new SimulatorTick(nullptr);
    tick->moveToThread(&tickThread);
    tickThread.start();

    // 跨线程设置网络和定时器依赖
    QMetaObject::invokeMethod(simulator, [simulator, network, tick]() {
        simulator->setNetwork(network);
        simulator->setTick(tick);
    }, Qt::BlockingQueuedConnection);

    // 初始化数据库（在数据库线程中执行）
    QString error;
    bool initOk = false;
    QMetaObject::invokeMethod(simulator, [simulator, &initOk, &error]() {
        initOk = simulator->initialize(&error);
    }, Qt::BlockingQueuedConnection);
    if(!initOk) {
        qCritical() << "cannot open charger database" << error;
        tickThread.quit(); tickThread.wait();
        netThread.quit(); netThread.wait();
        bizThread.quit(); bizThread.wait();
        dbThread.quit(); dbThread.wait();
        delete tick;
        delete network;
        delete simulator;
        delete database;
        return 1;
    }

    // 启动网络连接
    QMetaObject::invokeMethod(simulator, [simulator, &p]() {
        simulator->start(p.value("host"),
                         static_cast<quint16>(p.value("port").toUInt()));
    }, Qt::QueuedConnection);

    // 创建并显示主窗口
    SimWindow w(simulator);
    w.show();

    const int ret = app.exec();

    // 按创建反序退出线程并释放资源
    tickThread.quit(); tickThread.wait();
    netThread.quit(); netThread.wait();
    bizThread.quit(); bizThread.wait();
    dbThread.quit(); dbThread.wait();

    delete tick;
    delete network;
    delete simulator;
    delete database;

    return ret;
}
