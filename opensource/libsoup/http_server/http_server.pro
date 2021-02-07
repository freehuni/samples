TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp

INCLUDEPATH+=/usr/include/glib-2.0
#INCLUDEPATH+=/usr/lib/i386-linux-gnu/glib-2.0/include
INCLUDEPATH+=/usr/lib/x86_64-linux-gnu/glib-2.0/include

INCLUDEPATH+=/usr/include/libsoup-2.4

DEPENDPATH += /usr/lib/x86_64-linux-gnu/
LIBS += -lsoup-2.4
LIBS += -lgobject-2.0
LIBS += -lglib-2.0
