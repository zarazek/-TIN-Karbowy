#-------------------------------------------------
#
# Project created by QtCreator 2016-04-27T15:51:05
#
#-------------------------------------------------

QT       += core gui sql
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StacjaSzefa
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    checkboxdelegate.cpp \
    database.cpp

HEADERS  += mainwindow.h \
    checkboxdelegate.h \
    employee.h \
    database.h

FORMS    += mainwindow.ui
