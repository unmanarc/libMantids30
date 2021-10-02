QT       -= core gui
CONFIG += c++11

SOURCES += \
    src/globals.cpp \
    src/rpcclientapplication.cpp \
    src/rpcclientimpl.cpp \
    src/rpcwebserverapplication.cpp

HEADERS += \
    src/globals.h \
    src/rpcclientapplication.h \
    src/rpcclientimpl.h \
    src/rpcwebserverapplication.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib -lcx2_xrpc_fast2 -lcx2_thr_threads2 -lcx2_thr_mutex2 -lcx2_mem_vars2 -lcx2_prg_logs2 -lcx2_net_sockets2 -lcx2_prg_service2 -lcx2_hlp_functions2 -ljsoncpp


# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src

INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ Compiler Flags.
include(../../cflags.pri)

TARGET = cx2_xrpc_service
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
