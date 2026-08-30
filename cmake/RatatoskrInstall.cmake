install(TARGETS ${RATOS_INSTALL_TARGETS} EXPORT RatatoskrTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(RATOS_BUILD_CLI)
    install(TARGETS ratos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${PROJECT_SOURCE_DIR}/docs/man/ratos.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ratatoskr DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT RatatoskrTargets
    FILE RatatoskrTargets.cmake
    NAMESPACE Ratatoskr::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ratatoskr)

configure_file(${PROJECT_SOURCE_DIR}/packaging/ratatoskr.pc.in
    ${PROJECT_BINARY_DIR}/ratatoskr.pc @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/ratatoskr.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

configure_package_config_file(${PROJECT_SOURCE_DIR}/packaging/RatatoskrConfig.cmake.in
    ${PROJECT_BINARY_DIR}/RatatoskrConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ratatoskr)
write_basic_package_version_file(${PROJECT_BINARY_DIR}/RatatoskrConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${PROJECT_BINARY_DIR}/RatatoskrConfig.cmake
    ${PROJECT_BINARY_DIR}/RatatoskrConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Ratatoskr)
