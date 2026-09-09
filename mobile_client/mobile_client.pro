QT += core gui network qml quick quickcontrols2
CONFIG += c++11
TEMPLATE = app
TARGET = ev_mobile_client

SOURCES += main.cpp mobileclient.cpp
HEADERS += mobileclient.h apptheme.h \
    apptheme.h
RESOURCES += qml.qrc

android {
    QT += androidextras
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

include(../common/common.pri)
