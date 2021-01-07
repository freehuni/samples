TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        destination.cpp \
        filesource.cpp \
        logdestination.cpp \
        main.cpp \
        mediator.cpp \
        mediatorimpl.cpp \
        source.cpp

HEADERS += \
    destination.h \
    filesource.h \
    logdestination.h \
    mediator.h \
    mediatorimpl.h \
    source.h
