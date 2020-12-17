# NOT STABLE.
QT       -= core gui
CONFIG += c++11

SOURCES +=   \
    src/attributes.cpp \
    src/directory.cpp \
    src/filereader.cpp \
    src/h7file.cpp \
    src/link.cpp \
    src/permissions.cpp \
    src/registry.cpp \
    src/registryarray.cpp
HEADERS +=    \
    src/attributes.h \
    src/directory.h \
    src/filereader.h \
    src/h7file.h \
    src/link.h \
    src/permissions.h \
    src/registry.h \
    src/registryarray.h

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

TARGET = cx2_fs_hive7
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
