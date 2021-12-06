# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif(NOT DEFINED CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
)

execute_process(
  COMMAND conan export .
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.conan-recipes/llnl-units"
                    COMMAND_ECHO STDOUT
)

# Conan dependencies
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake")
  message(
    STATUS
      "Downloading conan.cmake from https://github.com/conan-io/cmake-conan"
  )
  file(
    DOWNLOAD
    "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake"
    EXPECTED_HASH
      SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
    TLS_VERIFY ON
  )
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/conan.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

conan_cmake_autodetect(conan_settings)
conan_cmake_install(
  PATH_OR_REFERENCE ${CMAKE_CURRENT_SOURCE_DIR} SETTINGS ${conan_settings}
  BUILD outdated
)

find_package(Benchmark)
find_package(Boost 1.67 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Sanitizers REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Python 3.7 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)
find_package(LLNL-Units REQUIRED)

# Generate files for free scipp API functions
include(scipp-functions)

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
else()
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/bigobj>)
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
    ${CMAKE_SOURCE_DIR}/tools/run-tests.sh.in
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run-tests.sh" COPYONLY
  )
  # Exclude cmake-generated source files
  set(COVERAGE_EXCLUDES
      "${CMAKE_CURRENT_BINARY_DIR}/core/*"
      "${CMAKE_CURRENT_BINARY_DIR}/variable/*.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/dataset/*.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/python/*.cpp"
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
    "${CMAKE_SOURCE_DIR}/tests"
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

set(EXPORT_NAME ${PROJECT_NAME}-targets)

if(DYNAMIC_LIB)
  install(
    EXPORT ${EXPORT_NAME}
    DESTINATION "lib/cmake/scipp"
    NAMESPACE scipp::
  )
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION lib/cmake/scipp
  )
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    VERSION "${CMAKE_PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion # switch to SameMajorVersion after 1.0
                                   # release
  )
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake"
    DESTINATION lib/cmake/scipp
  )
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.conan-recipes"
          DESTINATION lib/cmake/scipp
  )
endif(DYNAMIC_LIB)

include(scipp-install)

option(FULL_BUILD
       "Enable full build where the 'all' target includes tests and benchmarks."
       OFF
)

enable_testing()

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
add_subdirectory(python)
