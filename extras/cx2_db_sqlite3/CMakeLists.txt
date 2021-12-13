cmake_minimum_required(VERSION 3.10)

######################################
find_library (
    SQLITE3_LIB
    NAMES sqlite3 libsqlite3 # what to look for
)
if (NOT SQLITE3_LIB)
    message("-- WARNING: Did not found lib SQLITE3_LIB")
else()
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
    get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

    project(${LIB_NAME})
    project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "CX2 Framework Helper Functions")

    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

    file(GLOB_RECURSE EDV_INCLUDE_FILES "./*.h*")
    file(GLOB_RECURSE EDV_SOURCE_FILES "./*.c*")

    include(GNUInstallDirs)

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
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LIB_NAME}
            )

    install(
            FILES ${CMAKE_BINARY_DIR}/${PARENT_DIR}/${LIB_NAME}/${LIB_NAME}.pc
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
            )

    if (EXTRAPREFIX)
        target_include_directories(${LIB_NAME} PUBLIC ${EXTRAPREFIX}/include)
        target_link_libraries(${LIB_NAME} "-L${EXTRAPREFIX}/lib")
    endif()

    include_directories("${cx2_db_SOURCE_DIR}/../")
    target_link_libraries(${LIB_NAME} cx2_db)

    include_directories("${cx2_hlp_functions_SOURCE_DIR}/../")
    target_link_libraries(${LIB_NAME} cx2_hlp_functions)

    include_directories("${cx2_thr_mutex_SOURCE_DIR}/../")
    target_link_libraries(${LIB_NAME} cx2_thr_mutex)

    include_directories("${cx2_mem_vars_SOURCE_DIR}/../")
    target_link_libraries(${LIB_NAME} cx2_mem_vars)

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SQLite sqlite3)
    link_libraries(${SQLite_LIBRARIES})
    target_include_directories(${LIB_NAME} PUBLIC ${SQLite_INCLUDE_DIRS})
    target_link_libraries(${LIB_NAME} ${SQLite_LIBRARIES})

    message("-- SQLITE3_LIB found at [${SQLITE3_LIB}] for ${SQLite_LIBRARIES}")

#    target_link_libraries(${LIB_NAME} ${SQLITE3_LIB})

endif()
