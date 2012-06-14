#-------------------------------------------------
#
# Project created by QtCreator 2012-05-11T15:12:40
#
#-------------------------------------------------

QT       += core gui script scripttools

TARGET = pBlazSIM
TEMPLATE = app

win32 {
QMAKE_LFLAGS += -static-libgcc
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

FORMS    += mainwindow.ui

RESOURCES += resources.qrc

OTHER_FILES += \
    ../pBlazSIM-build-desktop-Qt_4_8_1_for_Desktop_-_MinGW__Qt_SDK__Debug/debug/IO.js
