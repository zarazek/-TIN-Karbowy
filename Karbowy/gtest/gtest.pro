#-------------------------------------------------
#
# Project created by QtCreator 2016-05-15T00:31:21
#
#-------------------------------------------------

QT       -= core gui

TARGET = gtest
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    gtest-all.cc

HEADERS +=
unix {
    target.path = /usr/lib
    INSTALLS += target
}
