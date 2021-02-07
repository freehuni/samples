#-------------------------------------------------
#
# Project created by QtCreator 2013-11-17T23:50:32
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = xmlTest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp

INCLUDEPATH += /usr/include/libxml2
LIBS+= /usr/lib/x86_64-linux-gnu/libxml2.so
