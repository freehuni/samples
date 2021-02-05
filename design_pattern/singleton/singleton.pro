TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp

# When std::call_once is called in some systgem, it causes system_error exception.
QMAKE_CFLAGS+=-pthread
