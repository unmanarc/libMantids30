QT       -= core gui

SOURCES += \
    src/cookies/http_cookie.cpp \
    src/helpers/http_hlp_chunked_retriever.cpp \
    src/containers/http_version.cpp \
    src/security/http_security_hsts.cpp \
    src/security/http_security_xframeopts.cpp \
    src/security/http_security_xssprotection.cpp \
    src/subparsers/http_content.cpp \
    src/subparsers/http_request.cpp \
    src/subparsers/http_status.cpp \
    src/httpv1_base.cpp \
    src/httpv1_server.cpp \
    src/httpv1_client.cpp \
    src/helpers/http_date.cpp \
    src/cookies/http_cookies_clientside.cpp \
    src/cookies/http_cookies_serverside.cpp \
    src/urlvars/http_urlvarcontent_subparser.cpp \
    src/urlvars/http_urlvars.cpp \
    src/urlvars/streamdecoder_url.cpp \
    src/urlvars/streamencoder_url.cpp

HEADERS += \
    src/cookies/http_cookie.h \
    src/helpers/http_hlp_chunked_retriever.h \
    src/defs/http_retcodes.h \
    src/containers/http_version.h \
    src/security/http_security_hsts.h \
    src/security/http_security_xframeopts.h \
    src/security/http_security_xssprotection.h \
    src/subparsers/http_content.h \
    src/subparsers/http_request.h \
    src/subparsers/http_status.h \
    src/httpv1_base.h \
    src/httpv1_server.h \
    src/httpv1_client.h \
    src/helpers/http_date.h \
    src/cookies/http_cookies_clientside.h \
    src/cookies/http_cookies_serverside.h \
    src/helpers/fullrequest.h \
    src/helpers/fullresponse.h \
    src/urlvars/http_urlvarcontent_subparser.h \
    src/urlvars/http_urlvars.h \
    src/urlvars/streamdecoder_url.h \
    src/urlvars/streamencoder_url.h

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

# includes dir
QMAKE_INCDIR += $$PREFIX/include
INCLUDEPATH += $$PREFIX/include

QMAKE_INCDIR += src
INCLUDEPATH += src

QMAKE_INCDIR += src/helpers
INCLUDEPATH += src/helpers

QMAKE_INCDIR += src/subparsers
INCLUDEPATH += src/subparsers

QMAKE_INCDIR += src/containers
INCLUDEPATH += src/containers

QMAKE_INCDIR += src/cookies
INCLUDEPATH += src/cookies

QMAKE_INCDIR += src/defs
INCLUDEPATH += src/defs

QMAKE_INCDIR += src/security
INCLUDEPATH += src/security

QMAKE_INCDIR += src/urlvars
INCLUDEPATH += src/urlvars

# C++ Compiler Flags.
include(../../cflags.pri)

win32:LIBS+= -L$$PREFIX/lib -lcx2_mem_vars2 -lcx2_netp_mime2 -lcx2_hlp_functions2

TARGET = cx2_netp_http
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
