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
    v2022.3.0
    VERSION
    2022.3.0
    SYSTEM
    YES
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

  # Boost headers (only container and iterator are used)
  set(BOOST_INCLUDE_LIBRARIES container iterator)
  cpmaddpackage(
    NAME
    Boost
    VERSION
    1.90.0
    URL
    https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
    URL_HASH
    SHA256=aca59f889f0f32028ad88ba6764582b63c916ce5f77b31289ad19421a96c555f
    SYSTEM
    YES
    OPTIONS
    "BOOST_SKIP_INSTALL_RULES OFF"
  )

  cpmaddpackage(
    NAME
    Eigen3
    VERSION
    3.4.0
    URL
    https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    URL_HASH
    SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
    SYSTEM
    YES
    OPTIONS
    "EIGEN_BUILD_DOC OFF"
    "BUILD_TESTING OFF"
    "EIGEN_BUILD_PKGCONFIG OFF"
  )
endif()
