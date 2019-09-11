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

find_package(Sanitizers REQUIRED)

include(boost)
include(Eigen)
include(pybind11)
include(GoogleTest)
include(GoogleBenchmark)
include(GenerateExportHeader)

if("${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}" VERSION_LESS "3.5")
  message(FATAL_ERROR "Python v3 interpreter must be greater than or equal to 3.5. Found ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
endif()

find_package(Boost 1.67 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")

option(WITH_CTEST "Enable ctest integration of tests" ON)
option(DYNAMIC_LIB "Build shared libraries" OFF)

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if (result)
  if (NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif (NOT CMAKE_INTERPROCEDURAL_OPTIMIZATION)
else ()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
  message(WARNING "IPO is not supported: ${output}")
endif (result)

# See https://github.com/pybind/pybind11/issues/1604
set(INTERPROCEDURAL_OPTIMIZATION_TESTS FALSE)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsized-deallocation")
  if(APPLE)
    add_compile_definitions(_APPLE_CLANG_)
  else()
    # For some reason clang6 on ubuntu18 can't link parts
    # with different INTERPROCEDURAL_OPTIMIZATION flag.
    set(INTERPROCEDURAL_OPTIMIZATION_TESTS ${CMAKE_INTERPROCEDURAL_OPTIMIZATION})
  endif()
else()
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  # Reduces binary size of, e.g., libscipp-core.so by several MB.
  set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
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
  message("Got version from Git: ${GIT_VERSION_INFO}")
  add_definitions(-DSCIPP_VERSION="${GIT_VERSION_INFO}")
endif()

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

add_subdirectory(common)
add_subdirectory(benchmark)
add_subdirectory(units)
add_subdirectory(core)
add_subdirectory(neutron)
add_subdirectory(python)
add_subdirectory(test)
# Custom target for building everyting. all excludes tests by default
add_custom_target(all-tests
                   DEPENDS 
		     all
		     scipp-common-test
                     scipp-core-test
		     scipp-unit-test
	             scipp-neutron-test)
