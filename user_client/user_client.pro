QT += core gui widgets network
CONFIG += c++11
TEMPLATE = app
TARGET = ev_user_client
SOURCES += main.cpp userwindow.cpp
HEADERS += userwindow.h
FORMS += userwindow.ui
include(../common/common.pri)

