QT       -= core gui

SOURCES +=  src/iauth_domains.cpp \
    src/iauth.cpp \
    src/iauth_methods_attributes.cpp \
    src/iauth_session.cpp \
    src/iauth_validation_account.cpp
HEADERS += \
    src/iauth.h \
    src/iauth_domains.h \
    src/iauth_methods_attributes.h \
    src/iauth_session.h \
    src/iauth_validation_account.h \
    src/datastructs/ds_auth_modes.h \
    src/datastructs/ds_auth_reasons.h \
    src/datastructs/ds_auth_passworddata.h \
    src/datastructs/ds_auth_passmodes.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src
QMAKE_INCDIR += src/datastructs
QMAKE_INCDIR += src/helpers

INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src
INCLUDEPATH += src/datastructs
INCLUDEPATH += src/helpers

# C++ standard.
include(../../cflags.pri)

TARGET = cx2_auth
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
