TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    main_test.cpp \
    mock_sample.cpp \
    tstory.cpp
LIBS+= -lgtest
LIBS+= -lgmock

LIBS += -lpthread
