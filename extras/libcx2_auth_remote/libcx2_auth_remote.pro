QT       -= core gui

SOURCES += \
    src/loginrpcclient.cpp \
    src/manager_remote_application.cpp \
    src/manager_remote.cpp \
    src/manager_remote_groups.cpp \
    src/manager_remote_attributes.cpp \
    src/manager_remote_accounts.cpp \
    src/manager_remote_passindexs.cpp
HEADERS += \
    src/loginrpcclient.h \
    src/manager_remote.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib -lcx2_net_sockets2 -lcx2_mem_vars2 -lcx2_auth2 -lcx2_thr_mutex2 -lcx2_hlp_functions2 -lcx2_thr_safecontainers2 -lcx2_xrpc_fast2 -ljsoncpp

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ Compiler Flags.
include(../../cflags.pri)

TARGET = cx2_auth_remote
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
