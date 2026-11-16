cmake_minimum_required(VERSION 3.10)

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

SET(RLIB_NAME ${LIB_NAME})
SET(LIB_NAME ${LIBPREFIX}_${PARENT_DIR}_${RLIB_NAME})

project(${LIB_NAME})
project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "Mantids30 Library for implementing a RESTful web server with sessions and WebSockets")

file(GLOB_RECURSE EDV_INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE EDV_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(${LIB_NAME} ${EDV_INCLUDE_FILES} ${EDV_SOURCE_FILES})

set_target_properties(  ${LIB_NAME}
                        PROPERTIES VERSION ${PROJECT_VERSION}
                        SOVERSION ${PROJECT_VERSION_MAJOR}
                        PUBLIC_HEADER "${EDV_INCLUDE_FILES}"
                      )

target_include_directories(${LIB_NAME}
    PRIVATE .
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(
        TARGETS ${LIB_NAME}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin
        ARCHIVE COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER COMPONENT dev DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIBPREFIX}/${PARENT_DIR}/${RLIB_NAME}
        )

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
        )

if (EXTRAPREFIX)
    target_include_directories(${LIB_NAME} PUBLIC ${EXTRAPREFIX}/include)
    target_link_libraries(${LIB_NAME} PUBLIC "-L${EXTRAPREFIX}/lib")
endif()

set(Mantids30_LIBRARIES
    Core_Threads
    Core_Helpers
    DataFormat_JWT
    Core_Memory
    Network_Sockets
    Protocol_HTTP
    Protocol_MIME
    Scripting_JSONExprEval
    Scripting_MantidsLang
    Runtime_Logs
    DataFormat_JSON
)

set(PC_REQUIRES "")
foreach(LIB ${Mantids30_LIBRARIES})
    target_link_libraries(${LIB_NAME} PUBLIC ${LIBPREFIX}_${LIB})
    string(APPEND PC_REQUIRES " ${LIBPREFIX}_${LIB}")
endforeach()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/genericlib.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}.pc" @ONLY)

################################################################################
# PKG-CONFIG INIT:
find_package(PkgConfig REQUIRED)
################################################################################

################################################################################

################################################################################
# Boost Packages:
find_package(Boost REQUIRED COMPONENTS regex json)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${LIB_NAME} PUBLIC Boost::regex Boost::json)
