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

install(DIRECTORY "src/scipp/" DESTINATION ${PYTHONDIR})

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
