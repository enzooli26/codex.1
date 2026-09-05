QT += core network
CONFIG += console c++11
CONFIG -= app_bundle
TEMPLATE = app
TARGET = ev_device_simulator
SOURCES += main.cpp simulator.cpp
HEADERS += simulator.h
include(../common/common.pri)

