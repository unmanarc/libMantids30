cmake_minimum_required(VERSION 3.10)

##############################################################################################################################
# Vars and Versions:
set(SVERSION 2.5.0)
set(PROJECT_HOMEPAGE_URL "https://github.com/unmanarc/cxFramework2")
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
project(cxFramework2)
project(${PROJECT_NAME} VERSION ${SVERSION} DESCRIPTION "CX2 C++11 Framework Libraries")
##############################################################################################################################

##############################################################################################################################
# Other CMake Global settings:
include(GNUInstallDirs)
##############################################################################################################################

#############################################################################################################################
# Subprojects:
ADD_SUBDIRECTORY(cx2Helpers)
ADD_SUBDIRECTORY(cx2Scripts)
ADD_SUBDIRECTORY(cx2Threads)
ADD_SUBDIRECTORY(cx2FS)
ADD_SUBDIRECTORY(cx2Mem)
ADD_SUBDIRECTORY(cx2DB)
ADD_SUBDIRECTORY(cx2Net)
ADD_SUBDIRECTORY(cx2NetParsers)
ADD_SUBDIRECTORY(cx2Auth)
ADD_SUBDIRECTORY(cx2Program)
ADD_SUBDIRECTORY(cx2RPC)
#ADD_SUBDIRECTORY(devel)
ADD_SUBDIRECTORY(extras)
#############################################################################################################################

