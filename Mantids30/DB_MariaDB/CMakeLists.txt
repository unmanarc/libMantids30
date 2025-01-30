cmake_minimum_required(VERSION 3.10)


######################################
find_library (
    MARIADB_LIB
    NAMES mariadb libmariadb mysql libmysql libmysqlclient mysqlclient # what to look for
    PATH_SUFFIXES mysql
)
if (NOT MARIADB_LIB)
    message("-- WARNING: Did not found lib MARIADB_LIB")
else()
    get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    get_filename_component(PARENT_DIRFULL ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
    get_filename_component(PARENT_DIR ${PARENT_DIRFULL} NAME)

    SET(RLIB_NAME ${LIB_NAME})
    SET(LIB_NAME ${LIBPREFIX}_${LIB_NAME})

    find_path (
            MYSQL_INCLUDE_DIR NAMES mysql.h PATHS /usr/include /usr/include/mysql /usr/include/mariadb
        )

    if (MYSQL_INCLUDE_DIR)
        message("-- MARIADB_LIB found at [${MARIADB_LIB}] for ${LIB_NAME}")
        message("-- MARIADB_INCLUDE_DIR found at [${MYSQL_INCLUDE_DIR}]")

        project(${LIB_NAME})
        project(${LIB_NAME} VERSION ${SVERSION} DESCRIPTION "Mantids30 Library for MariaDB Database Connection")

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
            Memory
            DB
            Threads
            Helpers
        )

        foreach(LIB ${Mantids30_LIBRARIES})
            include_directories("${Mantids30_${LIB}_SOURCE_DIR}/../../")
            target_link_libraries(${LIB_NAME} ${LIBPREFIX}_${LIB})
        endforeach()

        message("-- LINKING ${LIB_NAME} MARIADB_INCLUDE_DIR found at [${MYSQL_INCLUDE_DIR}]")

        target_include_directories(${LIB_NAME} PRIVATE ${MYSQL_INCLUDE_DIR})

        target_link_libraries(${LIB_NAME} ${MARIADB_LIB})
    else()
        message("-- WARNING: Did not found lib MARIADB_INCLUDE_DIR")
    endif()

endif()
