# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
cmake_minimum_required(VERSION 3.10.2)
project(scipp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(NOT CMAKE_BUILD_TYPE)
  set(
    CMAKE_BUILD_TYPE "Release"
    CACHE
      STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake"
    "${CMAKE_SOURCE_DIR}/CMake/sanitizers-cmake/cmake")

find_package(OpenMP 3.1 REQUIRED)
find_package(Sanitizers REQUIRED)

include(boost)
include(Eigen)
include(pybind11)
include(GoogleTest)
include(GoogleBenchmark)

if("${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" VERSION_LESS "3.5")
  message(FATAL_ERROR "Python v3 interpreter must be greater than or equal to 3.5. Found ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
endif()

find_package(Boost 1.67 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

# See https://github.com/pybind/pybind11/issues/1604
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsized-deallocation")
  if(APPLE)
    add_compile_definitions(_APPLE_CLANG_)
  endif()
endif()

option(WITH_CTEST "Enable ctest integration of tests" ON)

add_compile_options(-Wall
                    -Wextra
                    -Wpedantic
                    -Wunused
                    -Wshadow
                    -Winit-self
                    -Wpointer-arith
                    -Woverloaded-virtual
                    -Wold-style-cast
                    -Wcast-qual
                    -Wcast-align)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
                    $<$<COMPILE_LANGUAGE:CXX>:-fno-operator-names>)
# This is currently causing to many warnings, reenable when appropriate.
# add_compile_options ( -Wconversion )

enable_testing()

add_subdirectory(benchmark)
add_subdirectory(units)
add_subdirectory(core)
add_subdirectory(neutron)
add_subdirectory(python)
add_subdirectory(test)
