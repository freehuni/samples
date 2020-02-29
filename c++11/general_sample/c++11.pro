TEMPLATE = app
CONFIG += console c+11
CONFIG -= qt

SOURCES += main.cpp

#QMAKE_CXXFLAGS += -std=c++0x
QMAKE_CXXFLAGS +=-std=c++11

LIBS += -lpthread
