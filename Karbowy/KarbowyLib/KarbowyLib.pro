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
    systemerror.cpp \
    parse.cpp \
    protocolerror.cpp \
    task.cpp \
    timestamp.cpp

HEADERS +=\
        karbowylib_global.h \
    database.h \
    sockets.h \
    linebuffer.h \
    formatedexception.h \
    protocol.h \
    eventdispatcher.h \
    systemerror.h \
    concat.h \
    protocolerror.h \
    parse.h \
    timestamp.h \
    logentry.h \
    task.h

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

unix {
    target.path = /usr/lib
    INSTALLS += target
}

unix: PKGCONFIG += libcrypto++

QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_CFLAGS_RELEASE += -g

