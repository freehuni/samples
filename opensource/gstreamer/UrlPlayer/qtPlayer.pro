#-------------------------------------------------
#
# Project created by QtCreator 2013-09-14T15:55:45
#
#-------------------------------------------------


#[필수 library]
#  libgstreamer0.10-dev
#  libgstreamer-plugins-base0.10-dev


#[TS재생에 필요한 library]
#  gstreamer0.10-plugins-bad
#  gstreamer0.10-ffmpeg

QT       += core gui

TARGET = qtPlayer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui



# 아래 경로에 해당 so 가 없으면 각각 so를 설치한다.
LIBS+= /usr/lib/i386-linux-gnu/libglib-2.0.so
LIBS+= /usr/lib/i386-linux-gnu/libgobject-2.0.so

# no rdk env
LIBS+= /usr/lib/i386-linux-gnu/libgstreamer-0.10.so
LIBS+= /usr/lib/i386-linux-gnu/libgstinterfaces-0.10.so

# rdk env
#LIBS+= /usr/local/lib/libgstreamer-0.10.so
#LIBS+= /usr/local/lib/libgstinterfaces-0.10.so


INCLUDEPATH+=/usr/include/glib-2.0/
INCLUDEPATH+=/usr/lib/i386-linux-gnu/glib-2.0/include/
INCLUDEPATH+=/usr/include/libxml2/
INCLUDEPATH+=/usr/local/include/libxml2/

INCLUDEPATH+=/usr/include/gstreamer-0.10/

# rdk env
INCLUDEPATH+=/usr/local/include/gstreamer-0.10/
