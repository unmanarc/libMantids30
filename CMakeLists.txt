cmake_minimum_required(VERSION 3.10)

##############################################################################################################################
# Vars and Versions:
set(SVERSION 3.0.0)
set(LIBPREFIX "Mantids30")

set(PROJECT_HOMEPAGE_URL "https://github.com/unmanarc/libMantids30")
set(PROJECT_CONTACT "Aaron G. Mizrachi P. <aaron@unmanarc.com>")
##############################################################################################################################

##############################################################################################################################
# C++ Global Options Settings:
set(CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -pedantic -Weverything -Wno-c++98-compat -Wno-padded -Wno-documentation -Wno-unused-macros -Wno-undef -Wno-unused-parameters -Wno-sign-conversion -Wno-unsafe-buffer-usage -Wdeprecated -Wconversion -Wshadow")
##############################################################################################################################

##############################################################################################################################
# CMAKE Options:
option(SSLRHEL7 "OpenSSL 1.1 For Red Hat 7.x provided by EPEL" OFF)
option(BUILD_SHARED_LIBS "Enable building the library as a shared library instead of a static one." ON)

##############################################################################################################################

##############################################################################################################################
# Project Settings:
project(libMantids30)
project(${PROJECT_NAME} VERSION ${SVERSION} DESCRIPTION "Mini-Advanced Network Toolkit for Internet Services Development")
##############################################################################################################################

##############################################################################################################################
# Other CMake Global settings:
include(GNUInstallDirs)
##############################################################################################################################

#############################################################################################################################
# Subprojects:
ADD_SUBDIRECTORY(Mantids30)
#ADD_SUBDIRECTORY(devel)
#############################################################################################################################

