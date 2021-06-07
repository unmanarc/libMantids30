QT       -= core gui

SOURCES +=   src/authentication.cpp \
    src/json_streamable.cpp \
    src/methodsattributes_map.cpp \
    src/methodsmanager.cpp \
    src/multiauths.cpp

HEADERS += src/authentication.h \
    src/json_streamable.h \
    src/methodsattributes_map.h \
    src/methodsmanager.h \
    src/multiauths.h \
    src/retcodes.h \
    src/validation.h \
    src/validation_codes.h
isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib -ljsoncpp -lcx2_mem_vars2 -lcx2_thr_threads2 -lcx2_thr_mutex2 -lcx2_auth2
!win32:LIBS+= -ljsoncpp


# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ Compiler Flags.
include(../../cflags.pri)

TARGET = cx2_xrpc_common
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
