foreach(fuzzer packet name record)
    add_executable(ratos_fuzz_dns_${fuzzer} dns/fuzz_dns_${fuzzer}.c)
    target_include_directories(ratos_fuzz_dns_${fuzzer} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(ratos_fuzz_dns_${fuzzer} PRIVATE ratatoskr_static)
    target_compile_options(ratos_fuzz_dns_${fuzzer} PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(ratos_fuzz_dns_${fuzzer} PRIVATE -fsanitize=fuzzer,address,undefined)
endforeach()
