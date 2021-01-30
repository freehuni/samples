TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        igddevice.cpp \
        main.cpp \
        utility.cpp \
        menumanager.cpp


INCLUDEPATH+=/usr/include/gupnp-1.2/
INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/x86_64-linux-gnu/glib-2.0/include/
INCLUDEPATH+=/usr/include/gssdp-1.2/
INCLUDEPATH+=/usr/include/libsoup-2.4/
INCLUDEPATH+=/usr/include/libxml2/

LIBS+= -lglib-2.0
LIBS+= -lgobject-2.0
LIBS+= -lgupnp-1.2
LIBS+= -lgssdp-1.2
LIBS += -lreadline

LIBS+= -lpthread

HEADERS += \
    cli.h \
    igddevice.h \
    utility.h
