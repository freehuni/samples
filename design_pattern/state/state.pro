TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        brokenstate.cpp \
        lightdevice.cpp \
        main.cpp \
        offstate.cpp \
        onstate.cpp \
        state.cpp

HEADERS += \
    brokenstate.h \
    lightdevice.h \
    offstate.h \
    onstate.h \
    state.h
