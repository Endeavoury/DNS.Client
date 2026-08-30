function(ratos_configure_library target)
    target_compile_features(${target} PUBLIC c_std_11)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${PROJECT_SOURCE_DIR}/src)
    target_compile_definitions(${target} PRIVATE RATOS_BUILDING_LIBRARY)

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        target_link_libraries(${target} PRIVATE ws2_32 iphlpapi)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(RATOS_ENABLE_SANITIZERS)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            if("${target}" STREQUAL "ratatoskr_static")
                target_link_options(${target} INTERFACE -fsanitize=address,undefined)
            else()
                target_link_options(${target} PRIVATE -fsanitize=address,undefined)
            endif()
        endif()
    endif()

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME ratatoskr
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endfunction()

function(ratos_configure_executable target)
    target_compile_features(${target} PRIVATE c_std_11)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
