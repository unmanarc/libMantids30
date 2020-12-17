QT       -= core gui

SOURCES += \
    src/a_bin.cpp \
    src/a_bool.cpp \
    src/a_double.cpp \
    src/a_int16.cpp \
    src/a_int32.cpp \
    src/a_int64.cpp \
    src/a_int8.cpp \
    src/a_ipv4.cpp \
    src/a_ipv6.cpp \
    src/a_map.cpp \
    src/a_ptr.cpp \
    src/a_string.cpp \
    src/a_stringlist.cpp \
    src/a_uint16.cpp \
    src/a_uint32.cpp \
    src/a_uint64.cpp \
    src/a_uint8.cpp \
    src/a_var.cpp \
    src/a_varchar.cpp \
    src/b_base.cpp \
    src/b_chunks.cpp \
    src/b_mem.cpp \
    src/b_mmap.cpp \
    src/b_ref.cpp \
    src/filemap.cpp \
    src/nullcontainer.cpp \
    src/streamable.cpp \
    src/streamablefile.cpp \
    src/streamparser.cpp \
    src/substreamparser.cpp \
    src/vars.cpp
HEADERS += \
    src/a_allvars.h \
    src/a_bin.h \
    src/a_bool.h \
    src/a_double.h \
    src/a_int16.h \
    src/a_int32.h \
    src/a_int64.h \
    src/a_int8.h \
    src/a_ipv4.h \
    src/a_ipv6.h \
    src/a_map.h \
    src/a_ptr.h \
    src/a_string.h \
    src/a_stringlist.h \
    src/a_uint16.h \
    src/a_uint32.h \
    src/a_uint64.h \
    src/a_uint8.h \
    src/a_var.h \
    src/a_varchar.h \
    src/b_base.h \
    src/b_chunk.h \
    src/b_chunks.h \
    src/b_mem.h \
    src/b_mmap.h \
    src/b_ref.h \
    src/filemap.h \
    src/nullcontainer.h \
    src/streamable.h \
    src/streamablefile.h \
    src/streamparser.h \
    src/substreamparser.h \
    src/vars.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

!win32 {
    SOURCES +=
    HEADERS +=
}
win32 {
    SOURCES += src/win32/w32compat.cpp
    HEADERS += src/win32/w32compat.h
}

win32:LIBS+= -L$$PREFIX/lib -lcx2_thr_mutex2 -lcx2_thr_safecontainers2 -lcx2_hlp_functions2 -lws2_32

# includes dir
QMAKE_INCDIR += $$PREFIX/include
INCLUDEPATH += $$PREFIX/include

INCLUDEPATH += src/win32
QMAKE_INCDIR += src/win32

QMAKE_INCDIR += src
INCLUDEPATH += src
# C++ standard.
include(../../cflags.pri)

TARGET = cx2_mem_vars
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
