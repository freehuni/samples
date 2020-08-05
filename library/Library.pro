TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += UNIT_TEST

INCLUDEPATH += Logger
INCLUDEPATH += Utility
INCLUDEPATH += BaseThread

SOURCES += main.cpp \
    BaseThread/basethread.cpp \
    BaseThread/basethread_test.cpp \
    Logger/logger.cpp \
    Logger/logger_test.cpp \
    Utility/utility.cpp \
    Utility/utility_test.cpp \
    Utility/week.cpp \
    Utility/week_test.cpp \
    Logger/loggeroutput.cpp \
    Logger/loggeroutput_test.cpp \
    Logger/consoleoutput.cpp \
    Logger/colorconsoleoutput.cpp \
    Logger/fileoutput.cpp \
    Logger/udpoutput.cpp \
    Logger/fileoutput_test.cpp


HEADERS += ./BaseThread/basethread.h \
    BaseThread/basethread.h \
    Logger/logger.h \
    Utility/utility.h \
    Utility/week.h \
    Logger/loggeroutput.h \
    Logger/consoleoutput.h \
    Logger/colorconsoleoutput.h \
    Logger/fileoutput.h \
    Logger/udpoutput.h

LIBS += -lpthread
LIBS += -lgtest
