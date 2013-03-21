TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    pbPicoCore.c \
    pbLibgen.c \
    pBlazPCR.c

HEADERS += \
    version.h \
    pbTypes.h \
    pbPicoCore.h \
    pbLibgen.h

