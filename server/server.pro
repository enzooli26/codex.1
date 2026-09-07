QT += core network sql concurrent
CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = ev_server
SOURCES += main.cpp serverapp.cpp database.cpp
HEADERS += serverapp.h database.h
RESOURCES += resources.qrc
include(../common/common.pri)
