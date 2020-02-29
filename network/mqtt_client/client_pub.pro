TEMPLATE = app
CONFIG += console c++11
#CONFIG -= app_bundle
#CONFIG += qt

SOURCES += main.cpp

HEADERS+= /usr/include/mosquitto.h
INCLUDEPATH+=/usr/include/
LIBS+= /usr/lib/x86_64-linux-gnu/libmosquitto.so
