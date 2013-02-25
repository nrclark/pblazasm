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

include(qtpropertybrowser/src/qtpropertybrowser.pri)
include(extension/extension.pri)

SOURCES += main.cpp\
    mainwindow.cpp \
    projecthandler.cpp \
    settingshandler.cpp \
    qxmlsettings.cpp \
    psmhighlighter.cpp \
    loghighlighter.cpp \
    codeeditor.cpp

HEADERS  += mainwindow.h \
    projecthandler.h \
    settingshandler.h \
    qxmlsettings.h \
    psmhighlighter.h \
    loghighlighter.h \
    codeeditor.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
