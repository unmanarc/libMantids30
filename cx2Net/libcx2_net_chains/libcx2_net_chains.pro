QT       -= core gui

SOURCES += \
    src/chainprotocols/socketchain_aes.cpp \
    src/chainprotocols/socketchain_tls.cpp \
    src/chainprotocols/socketchain_xor.cpp \
    src/chainsockets.cpp \
    src/socketchainbase.cpp \
    src/socketchainendpointbase.cpp
HEADERS += \
    src/chainprotocols/socketchain_aes.h \
    src/chainprotocols/socketchain_tls.h \
    src/chainprotocols/socketchain_xor.h \
    src/chainsockets.h \
    src/socketchainbase.h \
    src/socketchainendpointbase.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ standard.
include(../../cflags.pri)

TARGET = cx2_net_chains
TEMPLATE = lib
# 
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
