QT       -= core gui

SOURCES += src/virtualnetworkinterface.cpp
HEADERS += src/virtualnetworkinterface.h

SOURCES += src/netifconfig.cpp
HEADERS += src/netifconfig.h

win32:HEADERS += src/tap-windows.h src/netheaders-windows.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ Compiler Flags.
include(../../cflags.pri)

win32:LIBS+= -L$$PREFIX/lib  -lws2_32 -liphlpapi

TARGET = cx2_net_interfaces
TEMPLATE = lib
# INSTALLATION:
target.path = $$PREFIX/lib
header_files.files = $$HEADERS
header_files.path = $$PREFIX/include/$$TARGET
INSTALLS += target
INSTALLS += header_files
# PKGCONFIG
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_LIBDIR = $$PREFIX/lib/
QMAKE_PKGCONFIG_INCDIR = $$PREFIX/include/$$TARGET
QMAKE_PKGCONFIG_CFLAGS = -I$$PREFIX/include/
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

include(../../version.pri)

HEADERS += \
    netifconfig.h
