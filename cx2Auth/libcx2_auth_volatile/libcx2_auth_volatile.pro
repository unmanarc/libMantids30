QT       -= core gui

SOURCES += \ 
    src/manager_volatile.cpp \
    src/manager_volatile_groups.cpp \
    src/manager_volatile_attributes.cpp \
    src/manager_volatile_accounts.cpp
HEADERS += \  
    src/manager_volatile.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}


win32:LIBS+= -L$$PREFIX/lib -lcx2_auth2 -lcx2_thr_mutex2 -lcx2_thr_safecontainers2

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ standard.
include(../../cflags.pri)

TARGET = cx2_auth_volatile
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
