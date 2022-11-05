cmake_minimum_required(VERSION 3.10)

##############################################################################################################################
# Vars and Versions:
set(SVERSION 2.8.1)

set(PROJECT_HOMEPAGE_URL "https://github.com/unmanarc/libMantids")
set(PROJECT_CONTACT "Aaron G. Mizrachi P. <aaron@unmanarc.com>")
##############################################################################################################################

##############################################################################################################################
# C++ Global Options Settings:
set(CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
##############################################################################################################################

##############################################################################################################################
# Project Settings:
project(libMantids)
project(${PROJECT_NAME} VERSION ${SVERSION} DESCRIPTION "Mini-Advanced Network Toolkit for Internet Services Development")
##############################################################################################################################

##############################################################################################################################
# Other CMake Global settings:
include(GNUInstallDirs)
##############################################################################################################################

#############################################################################################################################
# Subprojects:
ADD_SUBDIRECTORY(Helpers)
ADD_SUBDIRECTORY(Scripts)
ADD_SUBDIRECTORY(Threads)
ADD_SUBDIRECTORY(Filesystem)
ADD_SUBDIRECTORY(Memory)
ADD_SUBDIRECTORY(Database)
ADD_SUBDIRECTORY(Network)
ADD_SUBDIRECTORY(Protocols)
ADD_SUBDIRECTORY(Auth)
ADD_SUBDIRECTORY(Program)
ADD_SUBDIRECTORY(RPC)
#ADD_SUBDIRECTORY(devel)
ADD_SUBDIRECTORY(extras)
#############################################################################################################################

