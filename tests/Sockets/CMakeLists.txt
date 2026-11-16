find_package(Threads REQUIRED)
add_executable(sockets_test sockets_test.cpp)
target_include_directories(sockets_test PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(sockets_test PRIVATE Mantids30_Network_Sockets Threads::Threads ${CMAKE_DL_LIBS})
foreach(case lifecycle tcp_ipv4 tcp_ipv6 tcp_options tcp_failures tcp_reuse tcp_dns_fallback
             stream_transfer stream_errors stream_integers stream_delimiters stream_blocks buffered_reader
             interrupted_io shutdown_wakeup high_descriptors
             udp_ipv4 udp_ipv6 udp_errors datagram_ownership unix_sockets unix_backlog acceptor_pool acceptor_threads acceptor_limits immediate_stop debug_output)
    add_test(NAME sockets_${case} COMMAND sockets_test ${case})
    set_tests_properties(sockets_${case} PROPERTIES TIMEOUT 20 SKIP_RETURN_CODE 77 LABELS "network;sockets")
endforeach()
find_package(Python3 COMPONENTS Interpreter QUIET)
find_program(SOCKETS_OPENSSL_EXECUTABLE openssl)
if(Python3_Interpreter_FOUND AND SOCKETS_OPENSSL_EXECUTABLE)
    foreach(case exchange shutdown destructor untrusted hostname truncated server_timeout client_timeout default_trust insecure)
        add_test(NAME sockets_tls_${case}
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tls_fixture.py
                    $<TARGET_FILE:sockets_test> ${SOCKETS_OPENSSL_EXECUTABLE} ${case})
        set_tests_properties(sockets_tls_${case} PROPERTIES TIMEOUT 25 LABELS "network;sockets;tls")
    endforeach()
endif()
