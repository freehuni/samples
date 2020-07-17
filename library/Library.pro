TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    BaseThread/basethread.cpp \
    BaseThread/basethread_test.cpp \
    Logger/logger.cpp \
    Logger/logger_test.cpp \
    Utility/utility.cpp \
    Utility/utility_test.cpp


HEADERS += ./BaseThread/basethread.h \
    BaseThread/basethread.h \
    Logger/logger.h \
    Utility/utility.h

LIBS += -lpthread
LIBS += -lgtest
