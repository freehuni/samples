TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp

INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/i386-linux-gnu/glib-2.0/include/

LIBS+= /usr/lib/i386-linux-gnu/libglib-2.0.so
LIBS+= /usr/lib/i386-linux-gnu/libgobject-2.0.so

#LIBS+= /usr/local/lib/libglib-2.0.so
#LIBS+= /usr/local/lib/libgobject-2.0.so

HEADERS += \
    helloworld.h
