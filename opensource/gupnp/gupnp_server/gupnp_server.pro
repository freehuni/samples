TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp
SOURCES += utility.cpp
HEADERS += utility.h

LIBS+= /usr/lib/x86_64-linux-gnu/libglib-2.0.so
LIBS+= /usr/lib/x86_64-linux-gnu/libgobject-2.0.so
LIBS+= /usr/lib/x86_64-linux-gnu/libgupnp-1.0.so
LIBS+= -lgssdp-1.0

INCLUDEPATH+=/usr/include/gupnp-1.0/
INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/x86_64-linux-gnu/glib-2.0/include/
INCLUDEPATH+=/usr/include/gssdp-1.0/
INCLUDEPATH+=/usr/include/libsoup-2.4/
INCLUDEPATH+=/usr/include/libxml2/
