TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += main.cpp

INCLUDEPATH+=/usr/local/include/glib-2.0
INCLUDEPATH+=/usr/local/include/libsoup-2.4
INCLUDEPATH+=/usr/local/lib/glib-2.0/include


LIBS += /usr/local/lib/libgobject-2.0.so
LIBS += /usr/local/lib/libglib-2.0.so
LIBS += /usr/local/lib/libgio-2.0.so
LIBS += /usr/local/lib/libsoup-2.4.so

