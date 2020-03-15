TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

DEFINES += _POSIX
DEFINES += MICROSTACK_NOTLS

INCLUDEPATH +=  $$PWD/../cmake.build/include
CONFIG(debug, debug | release) {
    LIBS+= $$PWD/../cmake.build/lib/debug/libigdclient.so
} else {
    LIBS+= $$PWD/../cmake.build/lib/release/libigdclient.so
}
LIBS+= -lpthread
