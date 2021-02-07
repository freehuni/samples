TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp


INCLUDEPATH+=/usr/include/gstreamer-0.10/
INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/i386-linux-gnu/glib-2.0/include
INCLUDEPATH+=/usr/include/libxml2/

LIBS+= /usr/lib/i386-linux-gnu/libgstreamer-0.10.so
LIBS+= /usr/lib/i386-linux-gnu/libgobject-2.0.so


INCLUDEPATH+=/usr/local/include/gstreamer-1.0
#LIBS+=/usr/local/lib/libgstreamer-1.0.so


