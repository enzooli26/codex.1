QT += core gui widgets network sql
CONFIG += c++11
TEMPLATE = app
TARGET = ev_device_simulator
SOURCES += main.cpp simulator.cpp edgedatabase.cpp simwindow.cpp devicenetwork.cpp simulatortick.cpp
HEADERS += simulator.h edgedatabase.h simwindow.h devicenetwork.h simulatortick.h
FORMS += simwindow.ui
include(../common/common.pri)
