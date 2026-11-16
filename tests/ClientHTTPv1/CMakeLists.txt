find_package(Threads REQUIRED)
add_executable(client_httpv1_test client_test.cpp)
target_include_directories(client_httpv1_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(client_httpv1_test PRIVATE Mantids30_Client_HTTPv1 Threads::Threads)
add_test(NAME client_httpv1 COMMAND client_httpv1_test)
add_test(NAME http_content_trailers COMMAND client_httpv1_test --trailers)
add_test(NAME http_destination_write_errors COMMAND client_httpv1_test --write-errors)
set_tests_properties(client_httpv1 PROPERTIES TIMEOUT 60)

find_package(Python3 COMPONENTS Interpreter QUIET)
find_program(HTTPV1_OPENSSL_EXECUTABLE openssl)
if(Python3_Interpreter_FOUND AND HTTPV1_OPENSSL_EXECUTABLE)
    add_test(NAME client_httpv1_tls
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tls_test.py
                $<TARGET_FILE:client_httpv1_test> ${HTTPV1_OPENSSL_EXECUTABLE})
    set_tests_properties(client_httpv1_tls PROPERTIES TIMEOUT 60)
endif()
