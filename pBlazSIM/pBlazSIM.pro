
QT       += core gui script scripttools

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pBlazSIM
TEMPLATE = app

win32: {
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
}

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

