TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp


INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBS+= -lglib-2.0
LIBS+= -lpthread
