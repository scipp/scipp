# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif(NOT DEFINED CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
)

option(BENCHMARK "Enable benchmarks" OFF)

set(Python_FIND_IMPLEMENTATIONS CPython PyPy)
if(NOT SKIP_REMOTE_SOURCES)
  # cpm installs llnl units for conda and wheels both
  include(scipp-cpm)
endif()
find_package(units REQUIRED)
find_package(Boost 1.90 REQUIRED)
find_package(Eigen3 REQUIRED)
option(SANITIZERS "Enable sanitizers" OFF)
if(SANITIZERS)
  find_package(Sanitizers REQUIRED)
endif()
if(BENCHMARK)
  find_package(benchmark)
endif()
find_package(GTest CONFIG REQUIRED)
# libpython is not available on `manylinux` images, use `Development.Module`
# instead of `Development`
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

# Generate files for free scipp API functions
include(scipp-functions)

find_program(CCACHE ccache)
if(CCACHE)
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif()
message(STATUS "Tried to find ccache, found:")
message(STATUS ${CCACHE})

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# MP : Parallel compile, add before any targets so all use it
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:-MP>)

include(GoogleTest)
include(GenerateExportHeader)

option(THREADING "Build with multi-threading" ON)
if(THREADING)
  if((NOT SKBUILD OR SKIP_REMOTE_SOURCES) AND NOT TARGET TBB::tbb)
    find_package(TBB CONFIG REQUIRED)
  endif()
  configure_file(
    core/include/scipp/core/parallel-tbb.h core/include/scipp/core/parallel.h
    COPYONLY
  )
  # Set TBB runtime directory for Windows DLL copying Get the TBB location from
  # target properties (works for both CPM and find_package)
  if(TARGET TBB::tbb)
    get_target_property(
      TBB_IMPORTED_LOCATION TBB::tbb IMPORTED_LOCATION_RELEASE
    )
    if(NOT TBB_IMPORTED_LOCATION)
      get_target_property(TBB_IMPORTED_LOCATION TBB::tbb IMPORTED_LOCATION)
    endif()
    # For MSVC builds via CPM, TBB uses LIBRARY_OUTPUT_DIRECTORY
    if(NOT TBB_IMPORTED_LOCATION)
      get_target_property(
        TBB_IMPORTED_LOCATION TBB::tbb LIBRARY_OUTPUT_DIRECTORY
      )
    endif()
    if(TBB_IMPORTED_LOCATION)
      # If it's a file path, get the directory
      if(EXISTS "${TBB_IMPORTED_LOCATION}" AND NOT IS_DIRECTORY
                                               "${TBB_IMPORTED_LOCATION}"
      )
        get_filename_component(
          TBB_RUNTIME_DIR "${TBB_IMPORTED_LOCATION}" DIRECTORY
        )
      else()
        set(TBB_RUNTIME_DIR "${TBB_IMPORTED_LOCATION}")
      endif()
    endif()
  endif()
  if(DEFINED TBB_RUNTIME_DIR)
    message(STATUS "TBB runtime directory set to: ${TBB_RUNTIME_DIR}")
  else()
    message(STATUS "TBB runtime directory not set")
  endif()
else()
  configure_file(
    core/include/scipp/core/parallel-fallback.h
    core/include/scipp/core/parallel.h COPYONLY
  )
endif()

option(
  CTEST_DISCOVER_TESTS
  "Enable discoverage of *individual* tests by ctest. Test execution is slower, but ctest integration better. If OFF (default) each submodule test runner is registered as a *single* test."
  OFF
)
option(DYNAMIC_LIB "Build shared libraries" ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include(IPO)

# Optimization flags
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --param inline-unit-growth=50")
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
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:/bigobj> /EHsc /constexpr:steps2000000
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
    "--std=c++23"
    "--quiet"
    "--enable=style"
    "--inline-suppr"
    "--suppressions-list=${CMAKE_SOURCE_DIR}/cppcheck-suppressions.txt"
    "--cppcheck-build-dir=${CMAKE_BINARY_DIR}/cppcheck"
    "--error-exitcode=2"
    "--check-level=exhaustive"
  )
endif()

# This is currently causing to many warnings, re-enable when appropriate.
# add_compile_options ( -Wconversion )

# permissive- : Std compliant parsing, warnings (W3) set by CMake defaults
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:-permissive->)

# Setting this to ON seems to interfere with ccache, causing full recompilation,
# defaulting to OFF for now.
option(PRECOMPILED_HEADERS "Set whether to build with precompiled headers" OFF)

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
      "/*miniconda*"
      "/*micromamba*"
  )
  setup_target_for_coverage_lcov(
    NAME
    coverage
    EXECUTABLE
    sh
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/run-tests.sh"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    "${CMAKE_SOURCE_DIR}"
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
    DESTINATION lib/cmake/${PROJECT_NAME}
    NAMESPACE ${PROJECT_NAME}::
  )
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
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
          "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-cpm.cmake"
    DESTINATION lib/cmake/${PROJECT_NAME}
  )
endif(DYNAMIC_LIB)

include(scipp-install)
include(scipp-test)

if(DEFINED ENV{CONDA_BUILD})
  scipp_write_dist_info(VERSION "${SCIPP_VERSION_PEP440}")
endif()

option(FULL_BUILD
       "Enable full build where the 'all' target includes tests and benchmarks."
       OFF
)

add_subdirectory(common)
if(BENCHMARK)
  if(FULL_BUILD)
    add_subdirectory(benchmark)
  else()
    add_subdirectory(benchmark EXCLUDE_FROM_ALL)
  endif()
endif()
add_subdirectory(units)
add_subdirectory(core)
add_subdirectory(variable)
add_subdirectory(dataset)
add_subdirectory(test)
add_subdirectory(python)
