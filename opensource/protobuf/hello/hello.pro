#=========================================================
# 2013/7/24 mhkang2@humaxdigital.com
# Qt Creator project file
#=========================================================
QT -= core gui
TARGET = hello
CONFIG += console
CONFIG += qt
TEMPLATE = app
	
#QMAKE_CFLAGS += -0s
#QMAKE_CFLAGS += -Wall
DEFINES += _LARGEFILE64_SOURCE=1
DEFINES += SIZE_64BIT_SUPPORT 
DEFINES += _LARGEFILE_SOURCE
DEFINES += _FILE_OFFSET_BITS=64
DEFINES += _POSIX
	
SOURCES += $$PWD/./main.cpp
SOURCES += $$PWD/./cpp/hello.pb.cc
	
HEADERS += $$PWD/./cpp/hello.pb.h
	
INCLUDEPATH += $$PWD/..
LIBS += -lprotobuf
