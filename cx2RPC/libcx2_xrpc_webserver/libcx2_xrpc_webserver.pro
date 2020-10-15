QT       -= core gui

SOURCES += \
    src/resourcesfilter.cpp \
    src/sessionsmanager.cpp \
    src/webclienthandler.cpp \
    src/webserver.cpp
HEADERS += \
    src/resourcesfilter.h \
    src/sessionsmanager.h \
    src/webclienthandler.h \
    src/webserver.h
isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib -ljsoncpp -lboost_regex-mt-x32 -lcx2_thr_safecontainers2 -lcx2_thr_threads2 -lcx2_hlp_functions2 -lcx2_auth2 -lcx2_mem_vars2 -lcx2_net_sockets2 -lcx2_netp_http2 -lcx2_netp_mime2 -lcx2_xrpc_common2


# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ standard.
include(../../cflags.pri)

TARGET = cx2_xrpc_webserver
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
