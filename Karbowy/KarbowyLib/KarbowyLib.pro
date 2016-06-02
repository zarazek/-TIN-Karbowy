#-------------------------------------------------
#
# Project created by QtCreator 2016-05-14T22:47:03
#
#-------------------------------------------------

QT       -= core gui
CONFIG += c++14

TARGET = KarbowyLib
TEMPLATE = lib

DEFINES += KARBOWYLIB_LIBRARY

SOURCES += \
    database.cpp \
    sockets.cpp \
    linebuffer.cpp \
    formatedexception.cpp \
    protocol.cpp \
    eventdispatcher.cpp \
    systemerror.cpp

HEADERS +=\
        karbowylib_global.h \
    database.h \
    sockets.h \
    linebuffer.h \
    formatedexception.h \
    protocol.h \
    eventdispatcher.h \
    systemerror.h

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

unix {
    target.path = /usr/lib
    INSTALLS += target
}

unix: PKGCONFIG += libcrypto++

