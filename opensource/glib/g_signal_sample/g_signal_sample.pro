TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

INCLUDEPATH+=/usr/include/glib-2.0/


#QMAKE_CFLAGS	+= -m32
#QMAKE_CXXFLAGS	+= -m32
#QMAKE_LFLAGS	+= -m32

#INCLUDEPATH+=/usr/lib/i386-linux-gnu/glib-2.0/include/
#LIBS+= /usr/lib/i386-linux-gnu/libglib-2.0.so
#LIBS+= /usr/lib/i386-linux-gnu/libgobject-2.0.so

DEPENDPATH += /usr/lib/x86_64-linux-gnu/

INCLUDEPATH+=/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBS+= /usr/lib/x86_64-linux-gnu/libglib-2.0.so
LIBS+= /usr/lib/x86_64-linux-gnu/libgobject-2.0.so
