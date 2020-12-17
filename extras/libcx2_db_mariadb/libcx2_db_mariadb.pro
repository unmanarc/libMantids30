QT       -= core gui

SOURCES +=  \
    src/query_mariadb.cpp \
    src/sqlconnector_mariadb.cpp
HEADERS +=  \
    src/query_mariadb.h \
    src/sqlconnector_mariadb.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib 

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ standard.
include(../../cflags.pri)

win32:LIBS+= -L$$PREFIX/lib -lcx2_db -lcx2_thr_mutex2 -lcx2_mem_vars2 -lmariadb

TARGET = cx2_db_mariadb
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
