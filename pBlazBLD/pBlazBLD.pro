#-------------------------------------------------
#
# Project created by QtCreator 2013-01-15T11:27:41
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pBlazBLD
TEMPLATE = app

win32 {
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

include(../../../qtsolutions/qtpropertybrowser/src/qtpropertybrowser.pri)
include(extension/extension.pri)

LIBS += libqscintilla2

SOURCES += main.cpp\
    mainwindow.cpp \
    projecthandler.cpp \
    qxmlsettings.cpp

HEADERS  += mainwindow.h \
    projecthandler.h \
    qxmlsettings.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
