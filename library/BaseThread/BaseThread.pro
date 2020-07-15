TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    basethread.cpp \
    basethread_test.cpp

HEADERS += \
    basethread.h

LIBS += -lpthread
LIBS += -lgtest
