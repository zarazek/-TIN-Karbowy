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
    commongui.cpp

HEADERS  += mainwindow.h \
    task.h \
    employee.h \
    employeetablemodel.h \
    sortfilteremployeemodel.h \
    taskstablemodel.h \
    commongui.h

FORMS    += mainwindow.ui

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

LIBS += -L$$OUT_PWD/../KarbowyLib/ -lKarbowyLib

INCLUDEPATH += $$PWD/../KarbowyLib
DEPENDPATH += $$PWD/../KarbowyLib

unix: PKGCONFIG += libcrypto++
