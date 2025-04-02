# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
execute_process(
  COMMAND conan export .
  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.conan-recipes/llnl-units"
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
    "https://raw.githubusercontent.com/conan-io/cmake-conan/6e5369d13720f22e07e31bb6b9018dbe60529fea/conan.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/conan.cmake"
    EXPECTED_HASH
      SHA256=6abed7382c98a76b447675b30baff6a0b2f5d263041f6eb2fb682b80adaa2555
    TLS_VERIFY ON
  )
endif()

include(${CMAKE_CURRENT_BINARY_DIR}/conan.cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

if(SKBUILD)
  # The deploy generator is used to install dependencies into a know location.
  # The RUNTIME_DEPENDENCIES DIRECTORIES install option is then used to find the
  # dependencies. This is actually required only for Windows, since on Linux and
  # OSX cmake finds the files anyway, based on the target's rpath.
  set(CONAN_RUNTIME_DEPENDENCIES "tbb")
  conan_cmake_configure(
    REQUIRES
    benchmark/1.6.1
    boost/1.86.0
    eigen/3.4.0
    gtest/1.11.0
    LLNL-Units/0.12.3
    pybind11/2.13.5
    onetbb/2021.12.0
    OPTIONS
    benchmark:shared=False
    boost:header_only=True
    gtest:shared=False
    LLNL-Units:shared=False
    LLNL-Units:fPIC=True
    LLNL-Units:base_type=uint64_t
    LLNL-Units:namespace=llnl::units
    GENERATORS
    cmake_find_package_multi
    deploy
  )
else()
  conan_cmake_configure(
    REQUIRES
    LLNL-Units/0.12.3
    OPTIONS
    LLNL-Units:shared=False
    LLNL-Units:fPIC=True
    LLNL-Units:base_type=uint64_t
    LLNL-Units:namespace=llnl::units
    GENERATORS
    cmake_find_package_multi
  )
endif()

conan_cmake_autodetect(conan_settings)
if(DEFINED CMAKE_OSX_ARCHITECTURES)
  if("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
    # For Apple M1
    set(conan_settings "${conan_settings};arch=armv8")
  endif()
endif()
conan_cmake_install(
  PATH_OR_REFERENCE ${CMAKE_CURRENT_BINARY_DIR} SETTINGS ${conan_settings}
  BUILD outdated
)
