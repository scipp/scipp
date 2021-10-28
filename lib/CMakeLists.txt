# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif(NOT DEFINED CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE ccache)
if(CCACHE)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()

if(MSVC)
  set(CLCACHE_PATH
      ""
      CACHE PATH "Path to installed clcache executable directory"
  )
  if(EXISTS ${CLCACHE_PATH})
    set(CMAKE_VS_GLOBALS "CLToolExe=clcache.exe" "CLToolPath=${CLCACHE_PATH}"
                         "TrackFileAccess=false"
    )
    message(STATUS "Will attempt to use clcache")
  else(EXISTS ${CLCACHE_PATH})
    message(STATUS "CLCACHE_PATH not specified. Not enabled.")
    unset(CMAKE_VS_GLOBALS) # Clear all vs globals
  endif(EXISTS ${CLCACHE_PATH})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /constexpr:steps2000000")
endif(MSVC)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

execute_process(
  COMMAND conan export .
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/.conan-recipes/llnl-units"
                    COMMAND_ECHO STDOUT
)

# MP : Parallel compile, add before any targets so all use it
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:-MP>)

include(GoogleTest)
include(GenerateExportHeader)

option(DISABLE_MULTI_THREADING "Disable multi threading" OFF)
if(NOT DISABLE_MULTI_THREADING)
  find_package(TBB CONFIG)
endif()
if(TBB_FOUND)
  option(ENABLE_THREAD_LIMIT
         "Limit the number TBB threads. Limit number set via THREAD_LIMIT" OFF
  )
  if(ENABLE_THREAD_LIMIT)
    include(ProcessorCount)
    processorcount(N)
    set(THREAD_LIMIT
        N
        CACHE
          STRING
          "Maximum number of TBB threads if limit enabled via ENABLE_THREAD_LIMIT"
    )
  else()
    unset(THREAD_LIMIT)
  endif()
  configure_file(
    core/include/scipp/core/parallel-tbb.h core/include/scipp/core/parallel.h
  )
else()
  configure_file(
    core/include/scipp/core/parallel-fallback.h
    core/include/scipp/core/parallel.h COPYONLY
  )
endif()

option(WITH_CTEST "Enable ctest integration of tests" ON)
option(DYNAMIC_LIB "Build shared libraries" ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include(IPO)

# Optimization flags
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param inline-unit-growth=50")
endif()

# Get version from git-describe
execute_process(
  COMMAND git describe --tag
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  RESULT_VARIABLE HAVE_GIT_VERSION_INFO
  OUTPUT_VARIABLE GIT_VERSION_INFO OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(${HAVE_GIT_VERSION_INFO} EQUAL 0)
  message(STATUS "Got version from Git: ${GIT_VERSION_INFO}")
  add_definitions(-DSCIPP_VERSION="${GIT_VERSION_INFO}")
endif()

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wunused
    -Wshadow
    -Winit-self
    -Wpointer-arith
    -Woverloaded-virtual
    -Wold-style-cast
    -Wcast-qual
    -Wcast-align
    -Wno-unknown-warning-option # Avoiding clang warnings about unknown warning
    -Werror=attributes
    -Werror=maybe-uninitialized
    -Werror=uninitialized
    -Werror=return-type
  )
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-operator-names>
  )
endif()

option(CPPCHECK "Enable running cppcheck during compilation if found." OFF)
if(CPPCHECK)
  find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck REQUIRED)
endif()
if(CMAKE_CXX_CPPCHECK)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/cppcheck)
  list(
    APPEND
    CMAKE_CXX_CPPCHECK
    "--std=c++20"
    "--quiet"
    "--enable=style"
    "--inline-suppr"
    "--suppressions-list=${CMAKE_SOURCE_DIR}/cppcheck-suppressions.txt"
    "--cppcheck-build-dir=${CMAKE_BINARY_DIR}/cppcheck"
  )
endif()

# This is currently causing to many warnings, reenable when appropriate.
# add_compile_options ( -Wconversion )

# permissive- : Std compliant parsing, warnings (W3) set by CMake defaults
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:-permissive->)

# Setting this to ON seems to interfere with ccache, causing full recompilation,
# defaulting to OFF for now.
option(PRECOMPILED_HEADERS "Set whether to build with precompiled headers" OFF)

enable_testing()

# Custom target for building tests. all excludes tests by default
add_custom_target(all-tests)

option(COVERAGE "Enable code coverage run." OFF)
if(COVERAGE)
  include(CodeCoverage)
  configure_file(
    tools/run-tests.sh.in "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run-tests.sh"
    COPYONLY
  )
  # Exclude cmake-generated source files
  set(COVERAGE_EXCLUDES
      "${PROJECT_BINARY_DIR}/core/*"
      "${PROJECT_BINARY_DIR}/variable/*.cpp"
      "${PROJECT_BINARY_DIR}/dataset/*.cpp"
      "${PROJECT_BINARY_DIR}/python/*.cpp"
      "/usr/*"
      "/*conan/data*"
      "/*miniconda*"
  )
  setup_target_for_coverage_lcov(
    NAME
    coverage
    EXECUTABLE
    sh
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run-tests.sh"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    "${CMAKE_SOURCE_DIR}/python/tests"
    DEPENDENCIES
    install-scipp
    all-tests
  )
endif()

if(NOT APPLE)
  set(CMAKE_INSTALL_RPATH $ORIGIN)
else()
  set(CMAKE_INSTALL_RPATH @loader_path)
endif()

include(scipp-install)

add_subdirectory(common)
if(FULL_BUILD)
  add_subdirectory(benchmark)
else()
  add_subdirectory(benchmark EXCLUDE_FROM_ALL)
endif()
add_subdirectory(units)
add_subdirectory(core)
add_subdirectory(variable)
add_subdirectory(dataset)
add_subdirectory(test)
