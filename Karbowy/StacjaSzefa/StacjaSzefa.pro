#-------------------------------------------------
#
# Project created by QtCreator 2016-04-27T15:51:05
#
#-------------------------------------------------

QT       += core gui sql
CONFIG += c++14

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StacjaSzefa
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    employeetablemodel.cpp \
    sortfilteremployeemodel.cpp \
    taskstablemodel.cpp \
    commongui.cpp \
    listeners.cpp \
    server.cpp \
    clientconnection.cpp \
    predefinedqueries.cpp \
    logprocessor.cpp \
    taskassignmentdialog.cpp

HEADERS  += mainwindow.h \
    employee.h \
    employeetablemodel.h \
    sortfilteremployeemodel.h \
    taskstablemodel.h \
    commongui.h \
    clientconnection.h \
    server.h \
    predefinedqueries.h \
    serverlogentry.h \
    logprocessor.h \
    taskassignmentdialog.h

FORMS    += mainwindow.ui \
    taskassignmentdialog.ui

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

LIBS += -L$$OUT_PWD/../KarbowyLib/ -lKarbowyLib

INCLUDEPATH += $$PWD/../KarbowyLib
DEPENDPATH += $$PWD/../KarbowyLib

unix: PKGCONFIG += libcrypto++

QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_CFLAGS_RELEASE += -g
