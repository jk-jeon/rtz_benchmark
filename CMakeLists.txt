cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    rtz_benchmark
    VERSION 0.1.0
    DESCRIPTION "Benchmark several different trailing zero removal algorithms"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

docs_early_return()

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    rtz_benchmark_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    rtz_benchmark_lib ${warning_guard}
    PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(rtz_benchmark_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(rtz_benchmark_exe source/main.cpp)
add_executable(rtz_benchmark::exe ALIAS rtz_benchmark_exe)

set_target_properties(
    rtz_benchmark_exe PROPERTIES
    OUTPUT_NAME rtz_benchmark
    EXPORT_NAME exe
)

target_compile_features(rtz_benchmark_exe PRIVATE cxx_std_17)

target_link_libraries(rtz_benchmark_exe PRIVATE rtz_benchmark_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT rtz_benchmark_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of rtz_benchmark"
  )
endif()

include(cmake/dev-mode.cmake)
