QT       -= core gui
CONFIG   += c++11

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
    src/a_string.cpp \
    src/a_stringlist.cpp \
    src/a_uint16.cpp \
    src/a_uint32.cpp \
    src/a_uint64.cpp \
    src/a_uint8.cpp \
    src/abstract.cpp
HEADERS += \
    src/a_bin.h \
    src/a_bool.h \
    src/a_double.h \
    src/a_int16.h \
    src/a_int32.h \
    src/a_int64.h \
    src/a_int8.h \
    src/a_ipv4.h \
    src/a_ipv6.h \
    src/a_string.h \
    src/a_stringlist.h \
    src/a_uint16.h \
    src/a_uint32.h \
    src/a_uint64.h \
    src/a_uint8.h \
    src/abstract.h

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

TARGET = cx2_mem_abstracts
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
