QT       -= core gui

SOURCES += \
    src/manager_db_application.cpp \
    src/manager_db.cpp \
    src/manager_db_groups.cpp \
    src/manager_db_attributes.cpp \
    src/manager_db_accounts.cpp \
    src/manager_db_passindexs.cpp
HEADERS += \  
    src/manager_db.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

win32:LIBS+= -L$$PREFIX/lib -lcx2_mem_vars2 -lcx2_auth2 -lcx2_db2 -lcx2_thr_mutex2 -lcx2_hlp_functions2 -lcx2_thr_safecontainers2

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ standard.
include(../../cflags.pri)

TARGET = cx2_auth_db
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
