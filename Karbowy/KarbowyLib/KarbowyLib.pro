#-------------------------------------------------
#
# Project created by QtCreator 2016-05-14T22:47:03
#
#-------------------------------------------------

QT       -= gui
CONFIG += c++14

TARGET = KarbowyLib
TEMPLATE = lib

DEFINES += KARBOWYLIB_LIBRARY

SOURCES += \
    database.cpp \
    sockets.cpp \
    linebuffer.cpp

HEADERS +=\
        karbowylib_global.h \
    database.h \
    sockets.h \
    linebuffer.h

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

unix {
    target.path = /usr/lib
    INSTALLS += target
}

