QT       -= core gui

SOURCES += \
    src/acceptors/socket_acceptor_multithreaded.cpp \
    src/acceptors/socket_acceptor_poolthreaded.cpp \
    src/acceptors/socket_acceptor_thread.cpp \
    src/bridge/streamsocketsbridge.cpp \
    src/bridge/streamsocketsbridge_thread.cpp \
    src/datagrams/datagramsocket.cpp \
    src/socket.cpp \
    src/socket_tcp.cpp \
    src/socket_tls.cpp \
    src/socket_udp.cpp \
    src/streams/bufferedstreamreader.cpp \
    src/streams/streamsocket.cpp \
    src/streams/streamsocketreader.cpp \
    src/streams/streamsocketwriter.cpp

HEADERS += \
    src/acceptors/socket_acceptor_multithreaded.h \
    src/acceptors/socket_acceptor_poolthreaded.h \
    src/acceptors/socket_acceptor_thread.h \
    src/bridge/streamsocketsbridge.h \
    src/bridge/streamsocketsbridge_thread.h \
    src/datagrams/datagramsocket.h \
    src/socket.h \
    src/socket_tcp.h \
    src/socket_tls.h \
    src/socket_udp.h \
    src/socket_unix.h \
    src/streams/bufferedstreamreader.h \
    src/streams/streamsocket.h \
    src/streams/streamsocketreader.h \
    src/streams/streamsocketwriter.h

# Windows 17063 comes with unix sockets, but we are not supporting it yet.
!win32:SOURCES+=src/socket_unix.cpp
!win32:HEADERS+=src/socket_unix.h

win32:LIBS+= -L$$PREFIX/lib -lcx2_thr_threads2 -lcx2_mem_vars2 -lssl -lcrypto -lws2_32

# -lcx2_thr_mutex2 -lcx2_thr_safecontainers2 -lcx2_hlp_functions2

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

INCLUDEPATH += src/streams
QMAKE_INCDIR += src/streams

INCLUDEPATH += src/datagrams
QMAKE_INCDIR += src/datagrams

INCLUDEPATH += src/acceptors
QMAKE_INCDIR += src/acceptors

TARGET = cx2_net_sockets
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
