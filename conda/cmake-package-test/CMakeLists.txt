# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# ~~~
cmake_minimum_required(VERSION 3.16)
project(cmake-package-test)
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif(NOT DEFINED CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Conan dependencies
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(
    STATUS
      "Downloading conan.cmake from https://github.com/conan-io/cmake-conan"
  )
  file(
    DOWNLOAD
    "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
    "${CMAKE_BINARY_DIR}/conan.cmake"
    EXPECTED_HASH
      SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
    TLS_VERIFY ON
  )
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

conan_cmake_autodetect(conan_settings)
conan_cmake_install(
  PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR} SETTINGS ${conan_settings} BUILD
  outdated
)

set(PKG_VERSION
    "undefined"
    CACHE STRING "Package version to require."
)
find_package(scipp ${PKG_VERSION} REQUIRED)

add_executable(test main.cpp)
target_link_libraries(test PUBLIC scipp::dataset)
