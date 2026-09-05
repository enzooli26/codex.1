QT += core gui widgets network charts
CONFIG += c++11
TEMPLATE = app
TARGET = ev_admin_client
SOURCES += main.cpp adminwindow.cpp
HEADERS += adminwindow.h
FORMS += adminwindow.ui
include(../common/common.pri)
