QT += core gui widgets network webchannel positioning

# Qt WebEngine：系统安装 qtwebengine5-dev 时用标准模块；
# 无 root 环境下通过 QTE_DEV_DIR 指向本地解包的 dev 文件
QTE_DEV = $$(QTE_DEV_DIR)
!isEmpty(QTE_DEV) {
    INCLUDEPATH += $$QTE_DEV/usr/include/x86_64-linux-gnu/qt5 \
                   $$QTE_DEV/usr/include/x86_64-linux-gnu/qt5/QtWebEngineWidgets \
                   $$QTE_DEV/usr/include/x86_64-linux-gnu/qt5/QtWebEngine \
                   $$QTE_DEV/usr/include/x86_64-linux-gnu/qt5/QtWebEngineCore \
                   $$QTE_DEV/usr/include/x86_64-linux-gnu/qt5/QtWebChannel \
                   $$QTE_DEV/usr/include/x86_64-linux-gnu/qt5/QtPositioning
    LIBS += -L$$QTE_DEV/usr/lib/x86_64-linux-gnu \
            -lQt5WebEngine -lQt5WebEngineWidgets -lQt5WebEngineCore -lQt5WebChannel -lQt5Positioning
} else {
    QT += webenginewidgets webengine
}

CONFIG += c++11
TEMPLATE = app
TARGET = ev_user_client
SOURCES += main.cpp userwindow.cpp mapbridge.cpp maphttpserver.cpp
HEADERS += userwindow.h mapbridge.h maphttpserver.h
FORMS += userwindow.ui
RESOURCES += resources/map.qrc
include(../common/common.pri)

