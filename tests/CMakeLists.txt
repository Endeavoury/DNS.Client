add_executable(ratos_dns_tests dns/dns_tests.c)
target_include_directories(ratos_dns_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(ratos_dns_tests PRIVATE ratatoskr_static)
add_test(NAME ratos_dns_tests COMMAND ratos_dns_tests)

add_executable(ratos_abi_tests core/abi_tests.c)
target_link_libraries(ratos_abi_tests PRIVATE ratatoskr_static)
add_test(NAME ratos_abi_tests COMMAND ratos_abi_tests)

if(RATOS_BUILD_CLI)
    add_test(NAME ratos_cli_help COMMAND ratos --help)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME ratos_dns_udp_tcp_integration
            COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/integration/dns_fixture_test.py
                $<TARGET_FILE:ratos>)
    endif()
endif()
