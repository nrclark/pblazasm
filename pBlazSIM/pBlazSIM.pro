#-------------------------------------------------
#
# Project created by QtCreator 2012-05-11T15:12:40
#
#-------------------------------------------------

QT       += core gui

TARGET = pBlazSIM
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pBlaze.cpp \
    hexspinbox.cpp \
    ../qhexedit2/src/qhexedit.cpp \
    ../qhexedit2/src/qhexedit_p.cpp \
    ../qhexedit2/src/xbytearray.cpp \
    ../qhexedit2/src/commands.cpp \
    ioform.cpp

HEADERS  += mainwindow.h \
    pBlaze.h \
    hexspinbox.h \
    ../qhexedit2/src/xbytearray.h \
    ../qhexedit2/src/qhexedit_p.h \
    ../qhexedit2/src/qhexedit.h \
    ../qhexedit2/src/commands.h \
    ioform.h

FORMS    += mainwindow.ui \
    ioform.ui

RESOURCES += resources.qrc
