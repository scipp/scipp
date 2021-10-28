# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
cmake_minimum_required(VERSION 3.16)
execute_process(
  COMMAND git describe --tags --abbrev=0
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE SCIPP_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
project(
  scipp
  VERSION ${SCIPP_VERSION}
  LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE
  )
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"
                      "${CMAKE_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
)

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

find_package(Benchmark)
find_package(Boost 1.67 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Sanitizers REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Python 3.7 REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
find_package(LLNL-Units REQUIRED)

option(FULL_BUILD
       "Enable full build where the 'all' target includes tests and benchmarks."
       OFF
)

enable_testing()

# Custom install target for docs to depend on.
add_custom_target(
  install-scipp COMMAND cmake --build ${CMAKE_CURRENT_BINARY_DIR} --target
                        install
)

include(docs)
add_docs_target(docs BUILDER html DEPENDS install-scipp)
add_docs_target(doctest BUILDER doctest DEPENDS docs)
add_docs_target(linkcheck BUILDER linkcheck DEPENDS docs)

include(GNUInstallDirs)
set(SITE_PACKAGES_DIR
    ""
    CACHE STRING
          "Python site-packages directory to use for installing Python module."
)
if("${SITE_PACKAGES_DIR}" STREQUAL "")
  set(PYTHONDIR scipp)
  set(ARCHIVEDIR .)
else()
  file(TO_CMAKE_PATH ${SITE_PACKAGES_DIR}/scipp PYTHONDIR)
  file(TO_CMAKE_PATH ${SITE_PACKAGES_DIR} ARCHIVEDIR)
endif()
set(INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})

set(EXPORT_NAME ${PROJECT_NAME}-targets)

if(NOT APPLE)
  set(CMAKE_INSTALL_RPATH $ORIGIN)
else()
  set(CMAKE_INSTALL_RPATH @loader_path)
endif()

include(scipp-install)

add_subdirectory(lib)

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
