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
    checkboxdelegate.cpp

HEADERS  += mainwindow.h \
    checkboxdelegate.h

FORMS    += mainwindow.ui
