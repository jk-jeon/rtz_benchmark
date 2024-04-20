cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    rtz_benchmark
    VERSION 0.1.0
    DESCRIPTION "Benchmark several different trailing zero removal algorithms"
    LANGUAGES CXX
)

docs_early_return()

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare executable ----

add_executable(rtz_benchmark_exe source/main.cpp)
add_executable(rtz_benchmark::exe ALIAS rtz_benchmark_exe)

set_target_properties(
    rtz_benchmark_exe PROPERTIES
    OUTPUT_NAME rtz_benchmark
    EXPORT_NAME exe
)

target_compile_features(rtz_benchmark_exe PRIVATE cxx_std_20)

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
