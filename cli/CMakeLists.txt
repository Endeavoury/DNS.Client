add_executable(ratos
    main.c
    commands/dns.c
    commands/registry.c)

ratos_configure_executable(ratos)
target_include_directories(ratos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ratos PRIVATE ${RATOS_LINK_TARGET})
