TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp
SOURCES += ./BaseThread/basethread.cpp
SOURCES += ./BaseThread/basethread_test.cpp

HEADERS += ./BaseThread/basethread.h

LIBS += -lpthread
LIBS += -lgtest
