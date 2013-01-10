#-------------------------------------------------
#
# Project created by QtCreator 2012-05-11T15:12:40
#
#-------------------------------------------------

QT       += core gui script scripttools

TARGET = pBlazSIM
TEMPLATE = app

win32 {
QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

SOURCES  += \
    main.cpp \
    mainwindow.cpp \
    pBlaze.cpp \
    hexspinbox.cpp

HEADERS  += \
    mainwindow.h \
    pBlaze.h \
    hexspinbox.h

FORMS    += \
    mainwindow.ui

RESOURCES += \
    resources.qrc
