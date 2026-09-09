// 管理端客户端入口：创建应用并显示主窗口
#include <QApplication>
#include "adminwindow.h"
int main(int argc,char *argv[]){
    QApplication app(argc,argv);AdminWindow w;w.show();return app.exec();}

