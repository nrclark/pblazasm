#-------------------------------------------------
#
# Project created by QtCreator 2013-01-15T11:27:41
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pBlazBLD
TEMPLATE = app

win32 {
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

include(../../../qt-solutions-qt-solutions/qtpropertybrowser/src/qtpropertybrowser.pri)
include(extension/extension.pri)

LIBS += libqscintilla2

SOURCES += main.cpp\
        mainwindow.cpp \
    projecthandler.cpp

HEADERS  += mainwindow.h \
    projecthandler.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
