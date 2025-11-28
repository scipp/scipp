# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025 Scipp contributors (https://github.com/scipp)
# ~~~

# download CPM.cmake
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.42.0/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH
    SHA256=2020b4fc42dba44817983e06342e682ecfc3d2f484a581f11cc5731fbe4dce8a
)

include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

# Configure units library build type based on DYNAMIC_LIB option
if(DYNAMIC_LIB)
  set(UNITS_BUILD_SHARED "ON")
  set(UNITS_BUILD_STATIC "OFF")
  set(UNITS_BUILD_SHARED_LIBS "ON")
else()
  set(UNITS_BUILD_SHARED "OFF")
  set(UNITS_BUILD_STATIC "ON")
  set(UNITS_BUILD_SHARED_LIBS "OFF")
endif()

cpmaddpackage(
  NAME
  units
  GITHUB_REPOSITORY
  llnl/units
  VERSION
  0.13.1
  OPTIONS
  "UNITS_PROJECT_NAME units"
  "SKBUILD OFF"
  "UNITS_INSTALL ON"
  "BUILD_SHARED_LIBS ${UNITS_BUILD_SHARED_LIBS}"
  "UNITS_BUILD_STATIC_LIBRARY ${UNITS_BUILD_STATIC}"
  "UNITS_BUILD_SHARED_LIBRARY ${UNITS_BUILD_SHARED}"
  "UNITS_BUILD_OBJECT_LIBRARY OFF"
  "UNITS_BUILD_CONVERTER_APP OFF"
  "UNITS_BUILD_WEBSERVER OFF"
  "UNITS_BASE_TYPE uint64_t"
  "UNITS_ENABLE_TESTS OFF"
  "CMAKE_CXX_STANDARD 20"
)

if(SKBUILD)
  cpmaddpackage(
    NAME
    gtest
    GITHUB_REPOSITORY
    google/googletest
    GIT_TAG
    v1.15.0
    VERSION
    1.15.0
    OPTIONS
    "INSTALL_GTEST OFF"
    "gtest_force_shared_crt ON"
  )

  cpmaddpackage(
    NAME
    oneTBB
    GITHUB_REPOSITORY
    uxlfoundation/oneTBB
    GIT_TAG
    v2021.13.0
    VERSION
    2021.13.0
    OPTIONS
    "TBB_TEST OFF"
    "TBB_STRICT OFF"
    "TBBMALLOC_BUILD OFF"
  )

  cpmaddpackage(
    NAME
    benchmark
    GITHUB_REPOSITORY
    google/benchmark
    VERSION
    1.6.1
    OPTIONS
    "BENCHMARK_ENABLE_TESTING Off"
  )
endif()
