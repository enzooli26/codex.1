QT += core network sql
CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = ev_device_simulator
SOURCES += main.cpp simulator.cpp edgedatabase.cpp
HEADERS += simulator.h edgedatabase.h
include(../common/common.pri)
