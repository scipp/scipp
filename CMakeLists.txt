# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
cmake_minimum_required(VERSION 3.13)
project(scipp)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)
if(MSVC)
  set(CLCACHE_PATH
      ""
      CACHE PATH "Path to installed clcache executable directory"
  )
  if(EXISTS ${CLCACHE_PATH})
    if(NOT CMAKE_VERSION VERSION_LESS 3.13)
      set(CMAKE_VS_GLOBALS "CLToolExe=clcache.exe" "CLToolPath=${CLCACHE_PATH}"
                           "TrackFileAccess=false"
      )
      message(STATUS "Will attempt to use clcache")
    else(NOT CMAKE_VERSION VERSION_LESS 3.13)
      message(
        WARNING
          "CMake version ${CMAKE_VERSION} too low to support CLCACHE. Not enabled."
      )
    endif(NOT CMAKE_VERSION VERSION_LESS 3.13)
  else(EXISTS ${CLCACHE_PATH})
    message(STATUS "CLCACHE_PATH not specified. Not enabled.")
    unset(CMAKE_VS_GLOBALS) # Clear all vs globals
  endif(EXISTS ${CLCACHE_PATH})
endif(MSVC)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE
  )
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake"
                      "${CMAKE_SOURCE_DIR}/CMake/sanitizers-cmake/cmake"
)

find_package(Sanitizers REQUIRED)

# MP : Parallel compile, add before any targets so all use it
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:-MP>)

include(GTest)
include(boost)
include(Eigen)
include(pybind11)
include(GoogleBenchmark)
include(GenerateExportHeader)
find_package(Boost 1.67 REQUIRED)
find_package(TBB CONFIG)
if(TBB_FOUND)
  set(ENABLE_THREAD_LIMIT
      OFF
      CACHE
        BOOL
        "Limit the number TBB threads. Exact number to be specified via THREAD_LIMIT"
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

if("${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" VERSION_LESS "3.5")
  message(
    FATAL_ERROR
      "Python v3 interpreter must be greater than or equal to 3.5. Found ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}"
  )
endif()

option(WITH_CTEST "Enable ctest integration of tests" ON)
option(DYNAMIC_LIB "Build shared libraries" OFF)
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
  OUTPUT_VARIABLE GIT_VERSION_INFO
  OUTPUT_STRIP_TRAILING_WHITESPACE
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
  )
  add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-operator-names>
  )
endif(NOT WIN32)
# This is currently causing to many warnings, reenable when appropriate.
# add_compile_options ( -Wconversion )

# permissive- : Std compilant parsing, warnings (W3) set by CMake defaults
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:-permissive->)

enable_testing()

# Custom target for building tests. all excludes tests by default
add_custom_target(all-tests)
add_subdirectory(common)
add_subdirectory(benchmark)
add_subdirectory(units)
add_subdirectory(core)
add_subdirectory(variable)
add_subdirectory(dataset)
add_subdirectory(neutron)
add_subdirectory(python)
add_subdirectory(test)
