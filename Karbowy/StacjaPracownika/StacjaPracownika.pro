#-------------------------------------------------
#
# Project created by QtCreator 2016-05-22T11:19:58
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++14

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StacjaPracownika
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    communicationthread.cpp \
    predefinedqueries.cpp \
    logindialog.cpp

HEADERS  += mainwindow.h \
    communicationthread.h \
    predefinedqueries.h \
    logindialog.h

FORMS    += \
    mainwindow.ui \
    logindialog.ui

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += sqlite3

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../KarbowyLib/release/ -lKarbowyLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../KarbowyLib/debug/ -lKarbowyLib
else:unix: LIBS += -L$$OUT_PWD/../KarbowyLib/ -lKarbowyLib

INCLUDEPATH += $$PWD/../KarbowyLib
DEPENDPATH += $$PWD/../KarbowyLib

unix|win32: LIBS += -lboost_program_options

QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_CFLAGS_RELEASE += -g
