QT       -= core gui
CONFIG += c++11

SOURCES += \
    src/application.cpp \
    src/globalarguments.cpp \
    src/programvalues.cpp

HEADERS += \
    src/application.h \
    src/globalarguments.h \
    src/programvalues.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib -lcx2_thr_threads2 -lcx2_thr_mutex2 -lcx2_mem_vars2
#-lcx2_hlp_functions2 -lcx2_auth2 -lcx2_xrpc_common2
#-lcx2_thr_safecontainers2    -lcx2_net_sockets2 -lcx2_netp_http2 -lcx2_netp_mime2  -ljsoncpp -lboost_regex-mt-x32


# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src

INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ Compiler Flags.
include(../../cflags.pri)

TARGET = cx2_prg_service
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
