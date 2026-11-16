add_executable(webrestful_test test_main.cpp endpoint_cases.cpp policy_cases.cpp integration_cases.cpp state_cases.cpp)
target_include_directories(webrestful_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(webrestful_test PRIVATE Mantids30_Server_WebRESTFul)

foreach(suite routing security input cors filters sessions integration websocket)
    add_test(NAME webrestful_${suite} COMMAND webrestful_test ${suite})
    set_tests_properties(webrestful_${suite} PROPERTIES LABELS "webrestful;${suite}" TIMEOUT 60)
endforeach()
