TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../../library

SOURCES += main.cpp
LIBS += -ldl
# LIBS += ../../library/libhello.so
