
#-------------------------------------------------
#
# Project created by QtCreator 2013-01-15T11:27:41
#
#-------------------------------------------------

QT       += core gui widgets xml script scripttools

TARGET = pBlazSIM
TEMPLATE = app

win32 : QMAKE_LFLAGS += -static-libgcc -static-libstdc++

win32 : DEFINES += WIN32VERSION
mac : DEFINES += OSXVERSION


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    pBlaze.cpp \
    hexspinbox.cpp \
    pBlazeQt.cpp \
    qmtxhexinputdialog.cpp \
    qmtxpicoterm.cpp \
    qmtxlogbox.cpp \
    qmtxledbox.cpp \
    qmtxscriptuart.cpp \
    qmtxscriptcore.cpp \
    qscripthighlighter.cpp \
    codeeditor.cpp

HEADERS += \
    mainwindow.h \
    pBlaze.h \
    hexspinbox.h \
    pBlazeQt.h \
    qmtxhexinputdialog.h \
    qmtxpicoterm.h \
    qmtxlogbox.h \
    qmtxledbox.h \
    qmtxscriptuart.h \
    qmtxscriptcore.h \
    qscripthighlighter.h \
    codeeditor.h

FORMS += \
    mainwindow.ui \
    qmtxhexinputdialog.ui \
    qmtxledbox.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    AES.js \
    IO.js

