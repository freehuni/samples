TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    menumanager.cpp

LIBS += /usr/lib/x86_64-linux-gnu/libreadline.so

HEADERS += \
    menumanager.h
