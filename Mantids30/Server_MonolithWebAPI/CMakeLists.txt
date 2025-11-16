cmake_minimum_required(VERSION 3.10)

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

SET(RLIB_NAME ${LIB_NAME})
SET(LIB_NAME ${LIBPREFIX}_${LIB_NAME})

project(${LIB_NAME})
project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "Mantids30 Library for implementing a Monolith Web API Server")

file(GLOB_RECURSE EDV_INCLUDE_FILES "./*.h*")
file(GLOB_RECURSE EDV_SOURCE_FILES "./*.c*")

add_library(${LIB_NAME} ${EDV_INCLUDE_FILES} ${EDV_SOURCE_FILES})



set_target_properties(  ${LIB_NAME}
                        PROPERTIES VERSION ${PROJECT_VERSION}
                        SOVERSION 2
                        PUBLIC_HEADER "${EDV_INCLUDE_FILES}"
                      )

configure_file( "genericlib.pc.in" "${LIB_NAME}.pc" @ONLY )

target_include_directories(${LIB_NAME} PRIVATE .)

install(
        TARGETS ${LIB_NAME}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin
        ARCHIVE COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER COMPONENT dev DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIBPREFIX}/${RLIB_NAME}
        )

install(
        FILES ${CMAKE_BINARY_DIR}/${PARENT_DIR}/${RLIB_NAME}/${LIB_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
        )

if (EXTRAPREFIX)
    target_include_directories(${LIB_NAME} PUBLIC ${EXTRAPREFIX}/include)
    target_link_libraries(${LIB_NAME} "-L${EXTRAPREFIX}/lib")
endif()

set(Mantids30_LIBRARIES
    Threads
    Helpers
    Memory
    Net_Sockets
    Protocol_HTTP
    Protocol_MIME
    API_EndpointsAndSessions
    Server_WebCore
    Program_Logs
)

foreach(LIB ${Mantids30_LIBRARIES})
    include_directories("${Mantids30_${LIB}_SOURCE_DIR}/../../")
    target_link_libraries(${LIB_NAME} ${LIBPREFIX}_${LIB})
endforeach()

################################################################################
# PKG-CONFIG INIT:
find_package(PkgConfig REQUIRED)
################################################################################

################################################################################
# Find jsoncpp
pkg_check_modules(JSONCPP jsoncpp)
link_libraries(${JSONCPP_LIBRARIES})
target_include_directories(${LIB_NAME} PUBLIC ${JSONCPP_INCLUDE_DIRS})
target_link_libraries(${LIB_NAME} ${JSONCPP_LIBRARIES})

################################################################################
# Boost Packages:
find_package(Boost REQUIRED COMPONENTS regex)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${LIB_NAME} ${Boost_LIBRARIES})
