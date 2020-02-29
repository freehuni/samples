TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp


INCLUDEPATH += /usr/include/gssdp-1.0
INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/x86_64-linux-gnu/glib-2.0/include/

LIBS+= /usr/lib/x86_64-linux-gnu/libgssdp-1.0.so
LIBS+= /usr/lib/x86_64-linux-gnu/libgio-2.0.so
LIBS+= /usr/lib/x86_64-linux-gnu/libgobject-2.0.so
LIBS+=/usr/lib/x86_64-linux-gnu/libglib-2.0.so
