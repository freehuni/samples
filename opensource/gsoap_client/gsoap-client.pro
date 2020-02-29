#=========================================================
# 2013/7/24 mhkang2@humaxdigital.com
# Qt Creator project file
#=========================================================
QT -= core gui
TARGET = gsoap-client
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
DEFINES += CONFIG_HOMMA_DMR
DEFINES += CONFIG_HOMMA_DMS
DEFINES += CONFIG_HOMMA_SATIP
#DEFINES += more...
	
SOURCES += $$PWD/./soapcalcProxy.cpp
SOURCES += $$PWD/./soapC.cpp
SOURCES += $$PWD/./calcclient.cpp
	
HEADERS += $$PWD/./soapStub.h
HEADERS += $$PWD/./soapcalcProxy.h
HEADERS += $$PWD/./calc.h
HEADERS += $$PWD/./soapH.h
	
INCLUDEPATH += $$PWD/..
LIBS += -lgsoap++
