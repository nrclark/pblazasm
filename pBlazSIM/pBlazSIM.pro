#-------------------------------------------------
#
# Project created by QtCreator 2012-05-11T15:12:40
#
#-------------------------------------------------

QT       += core gui script scripttools

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pBlazSIM
TEMPLATE = app

win32 {
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

LIBS += libqscintilla2

SOURCES  += \
    main.cpp \
    mainwindow.cpp \
    pBlaze.cpp \
    hexspinbox.cpp \
    pBlazeIO.cpp

HEADERS  += \
    mainwindow.h \
    pBlaze.h \
    hexspinbox.h \
    pBlazeIO.h

FORMS    += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
