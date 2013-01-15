#-------------------------------------------------
#
# Project created by QtCreator 2013-01-15T11:27:41
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pBlazBLD
TEMPLATE = app

LIBS += libqscintilla2

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
