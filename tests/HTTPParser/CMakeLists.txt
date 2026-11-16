add_executable(http_parser_test parser_test.cpp client_cases.cpp server_cases.cpp)
target_include_directories(http_parser_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(http_parser_test PRIVATE Mantids30_Protocol_HTTP)

foreach(suite client_valid client_invalid client_limits server_valid server_invalid server_limits streaming)
    add_test(NAME http_parser_${suite} COMMAND http_parser_test ${suite})
    set_tests_properties(http_parser_${suite} PROPERTIES LABELS "http;parser;${suite}" TIMEOUT 60)
endforeach()
