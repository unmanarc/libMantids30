cmake_minimum_required(VERSION 3.10)

get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

project(${LIB_NAME})
project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "CX2 Framework MIME Decoder/Encoder Protocol")

file(GLOB_RECURSE EDV_INCLUDE_FILES "./*.h*")
file(GLOB_RECURSE EDV_SOURCE_FILES "./*.c*")

add_library(${LIB_NAME} ${EDV_INCLUDE_FILES} ${EDV_SOURCE_FILES})



set_target_properties(  ${LIB_NAME}
                        PROPERTIES VERSION ${PROJECT_VERSION}
                        SOVERSION 2
                        PUBLIC_HEADER "${EDV_INCLUDE_FILES}"
                      )

configure_file(
                "genericlib.pc.in"
                "${LIB_NAME}.pc" @ONLY
              )

target_include_directories(${LIB_NAME} PRIVATE .)

install(
        TARGETS ${LIB_NAME}
        ARCHIVE COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY COMPONENT lib DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER COMPONENT dev DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
        )

install(
        FILES ${CMAKE_BINARY_DIR}/${PARENT_DIR}/${LIB_NAME}/${LIB_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
        )

if (EXTRAPREFIX)
    target_include_directories(${LIB_NAME} PUBLIC ${EXTRAPREFIX}/include)
    target_link_libraries(${LIB_NAME} "-L${EXTRAPREFIX}/lib")
endif()

include_directories("${cx2_mem_vars_SOURCE_DIR}/../")
target_link_libraries(${LIB_NAME} cx2_mem_vars)

include_directories("${cx2_hlp_functions_SOURCE_DIR}/../")
target_link_libraries(${LIB_NAME} cx2_hlp_functions)

include_directories("${cx2_thr_threads_SOURCE_DIR}/../")
target_link_libraries(${LIB_NAME} cx2_thr_threads)

include_directories("${cx2_thr_mutex_SOURCE_DIR}/../")
target_link_libraries(${LIB_NAME} cx2_thr_mutex)


######################################

find_library (
    BOOST_REGEX_LIB
    NAMES boost_regex-mt-x32 libboost_regex-mt-x32 boost_regex-mt libboost_regex-mt boost_regex libboost_regex # what to look for
)
if (NOT BOOST_REGEX_LIB)
    message("ERROR: Did not found lib BOOST_REGEX_LIB")
else()
    message("-- BOOST_REGEX_LIB found at [${BOOST_REGEX_LIB}] for ${LIB_NAME}")
endif()

target_link_libraries(${LIB_NAME} ${BOOST_REGEX_LIB})

####################################

find_library (
    BOOST_THREAD_LIB
    NAMES boost_thread-mt-x32 libboost_thread-mt-x32 boost_thread-mt libboost_thread-mt boost_thread libboost_thread # what to look for
)
if (NOT BOOST_THREAD_LIB)
    message("ERROR: Did not found lib BOOST_THREAD_LIB")
else()
    message("-- BOOST_THREAD_LIB found at [${BOOST_THREAD_LIB}] for ${LIB_NAME}")
endif()

target_link_libraries(${LIB_NAME} ${BOOST_THREAD_LIB})

