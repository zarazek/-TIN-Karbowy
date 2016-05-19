TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    linebuffer.cpp \
    gtest_main.cc \
    sockets.cpp

LIBS += -L$$OUT_PWD/../KarbowyLib/ -lKarbowyLib

INCLUDEPATH += $$PWD/../KarbowyLib
DEPENDPATH += $$PWD/../KarbowyLib

LIBS += -L$$OUT_PWD/../gtest/ -lgtest

INCLUDEPATH += $$PWD/../gtest
DEPENDPATH += $$PWD/../gtest

PRE_TARGETDEPS += $$OUT_PWD/../gtest/libgtest.a

LIBS += -lpthread
