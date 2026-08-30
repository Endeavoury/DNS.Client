add_subdirectory(core)
add_subdirectory(protocols)

set(RATOS_SOURCES ${RATOS_CORE_SOURCES} ${RATOS_PROTOCOL_SOURCES})

if(RATOS_BUILD_SHARED)
    add_library(ratatoskr SHARED ${RATOS_SOURCES})
    ratos_configure_library(ratatoskr)
    set(RATOS_LINK_TARGET ratatoskr PARENT_SCOPE)
    list(APPEND RATOS_INSTALL_TARGETS ratatoskr)
endif()

if(RATOS_BUILD_STATIC)
    add_library(ratatoskr_static STATIC ${RATOS_SOURCES})
    ratos_configure_library(ratatoskr_static)
    set_target_properties(ratatoskr_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(ratatoskr_static PUBLIC RATOS_STATIC)
    if(RATOS_BUILD_FUZZERS)
        target_compile_options(ratatoskr_static PRIVATE
            -fsanitize=fuzzer-no-link,address,undefined
            -fno-omit-frame-pointer)
    endif()
    if(NOT RATOS_BUILD_SHARED)
        set(RATOS_LINK_TARGET ratatoskr_static PARENT_SCOPE)
    endif()
    list(APPEND RATOS_INSTALL_TARGETS ratatoskr_static)
endif()

set(RATOS_INSTALL_TARGETS ${RATOS_INSTALL_TARGETS} PARENT_SCOPE)
