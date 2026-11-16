find_package(Boost REQUIRED COMPONENTS json)
add_executable(core_memory_test
    test_main.cpp memory_test.cpp containers_test.cpp files_test.cpp
    streams_test.cpp parsers_test.cpp variables_test.cpp)
target_include_directories(core_memory_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(core_memory_test PRIVATE Mantids30_Core_Memory Boost::json)
foreach(suite helpers containers files streams parsers variables)
    add_test(NAME core_memory_${suite} COMMAND core_memory_test ${suite})
    set_tests_properties(core_memory_${suite} PROPERTIES LABELS "memory;${suite}" TIMEOUT 60)
endforeach()
