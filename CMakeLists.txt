cmake_minimum_required(VERSION 3.20)
project(Ratatoskr VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(RatatoskrOptions)
include(RatatoskrCompiler)

add_subdirectory(src)

if(RATOS_BUILD_CLI)
    add_subdirectory(cli)
endif()
if(RATOS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
if(RATOS_BUILD_FUZZERS)
    add_subdirectory(fuzz)
endif()

include(RatatoskrInstall)
