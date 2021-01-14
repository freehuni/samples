TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    main_test.cpp
LIBS+= -lgtest
LIBS += -lpthread
