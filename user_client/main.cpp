#include <QApplication>
#include <QtWebEngine/QtWebEngine>
#include "userwindow.h"
int main(int argc,char *argv[]){
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QtWebEngine::initialize();
    QApplication app(argc,argv);
    UserWindow w;w.show();
    return app.exec();
}

