QT += core gui network qml quick quickcontrols2
CONFIG += c++11
TEMPLATE = app
TARGET = ev_mobile_client

SOURCES += main.cpp mobileclient.cpp
HEADERS += mobileclient.h
RESOURCES += qml.qrc

android {
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
}

include(../common/common.pri)
