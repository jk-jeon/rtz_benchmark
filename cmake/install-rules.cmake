if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_INSTALL_INCLUDEDIR include/rtz_benchmark CACHE PATH "")
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package rtz_benchmark)

install(
    TARGETS rtz_benchmark_exe
    EXPORT rtz_benchmarkTargets
    RUNTIME COMPONENT rtz_benchmark_Runtime
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    rtz_benchmark_INSTALL_CMAKEDIR "${CMAKE_INSTALL_DATADIR}/${package}"
    CACHE PATH "CMake package config location relative to the install prefix"
)
mark_as_advanced(rtz_benchmark_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${rtz_benchmark_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT rtz_benchmark_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${rtz_benchmark_INSTALL_CMAKEDIR}"
    COMPONENT rtz_benchmark_Development
)

install(
    EXPORT rtz_benchmarkTargets
    NAMESPACE rtz_benchmark::
    DESTINATION "${rtz_benchmark_INSTALL_CMAKEDIR}"
    COMPONENT rtz_benchmark_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
