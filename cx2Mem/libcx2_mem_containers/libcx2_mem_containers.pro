QT       -= core gui
CONFIG += c++11

SOURCES += \
    src/b_base.cpp \
    src/b_mmap.cpp \
    src/b_chunks.cpp \
    src/b_mem.cpp \
    src/b_ref.cpp \
    \ # src/common.cpp #\
    src/nullcontainer.cpp
#    src/b_file.cpp
HEADERS +=\
    src/b_chunk.h \
    src/b_base.h \
    src/b_mmap.h \
    src/b_chunks.h \
    src/b_mem.h \
    src/b_ref.h \
    src/b_filereference.h \ #\
    src/nullcontainer.h
#    src/b_file.h
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

# LIB DEFS:
TARGET = cx2_mem_containers
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
