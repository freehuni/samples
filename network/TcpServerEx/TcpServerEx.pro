TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    TcpAcceptor.cpp

HEADERS += \
    TcpAcceptor.h \
    mytypes.h


LIBS += -lpthread
