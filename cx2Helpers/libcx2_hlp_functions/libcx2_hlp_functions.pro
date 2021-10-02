QT       -= core gui
CONFIG += c++11

SOURCES += src/encoders.cpp \
           src/json.cpp \
           src/os.cpp \
           src/random.cpp \
           src/mem.cpp \
           src/crypto.cpp

HEADERS += src/encoders.h \
           src/json.h \
           src/os.h \
           src/random.h \
           src/mem.h \
           src/crypto.h \
           src/stdlist.h \
           src/stdmap.h \
           src/stdset.h \
           src/stdvector.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# includes dir
QMAKE_INCDIR += $$PREFIX/include
QMAKE_INCDIR += src

INCLUDEPATH += $$PREFIX/include
INCLUDEPATH += src

# C++ Compiler Flags.
include(../../cflags.pri)

win32:LIBS+= -L$$PREFIX/lib -lcrypto -ljsoncpp

TARGET = cx2_hlp_functions
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
