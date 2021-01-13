TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        directory.cpp \
        entry.cpp \
        file.cpp \
        main.cpp \
        visitor.cpp

HEADERS += \
    directory.h \
    entry.h \
    file.h \
    visitor.h
