# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
cmake_minimum_required(VERSION 3.28)

cmake_policy(SET CMP0091 NEW)

# Dont scan for modules, we aren't using them right now and compilers for osx on
# conda don't seem to play well with this.
set(CMAKE_CXX_SCAN_FOR_MODULES 0)

if(SKBUILD)
  set(SCIPP_VERSION_PEP440 "${SKBUILD_PROJECT_VERSION_FULL}")
  set(SCIPP_VERSION_CMAKE "${SKBUILD_PROJECT_VERSION}")
  message(STATUS "Got version from SKBUILD: ${SKBUILD_PROJECT_VERSION_FULL}")
elseif(CONDA_FORGE_BUILD)
  set(SCIPP_VERSION_PEP440 "$ENV{GIT_VERSION_INFO}")
  set(SCIPP_VERSION_CMAKE "$ENV{GIT_VERSION_INFO}")
  message(
    STATUS "Got version from env var from conda-forge: ${SCIPP_VERSION_PEP440}"
  )
else()
  execute_process(
    COMMAND git describe --tags --exclude nightly
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE HAVE_GIT_VERSION_INFO
    OUTPUT_VARIABLE GIT_VERSION_INFO
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT HAVE_GIT_VERSION_INFO EQUAL 0)
    message(WARNING "Failed to get package version")
  endif()
  # We get something like 23.08.0-266-g068b161fe, remove the -g<hash> part
  string(REGEX REPLACE "-g[0-9a-f]+$" "" GIT_VERSION_INFO ${GIT_VERSION_INFO})
  # Replace '-' with '.dev' for PEP440 compatibility
  string(REGEX REPLACE "-" ".dev" SCIPP_VERSION_PEP440 ${GIT_VERSION_INFO})
  # Cmake does not like letters, remove the dev, will act as "tweak" number
  string(REGEX REPLACE "-" "" SCIPP_VERSION_CMAKE ${GIT_VERSION_INFO})
  unset(HAVE_GIT_VERSION_INFO)
  unset(GIT_VERSION_INFO)
  message(STATUS "Got version from git: ${SCIPP_VERSION_PEP440}")
endif()

project(
  scipp
  VERSION ${SCIPP_VERSION_CMAKE}
  LANGUAGES CXX
)
add_definitions(-DSCIPP_VERSION="${SCIPP_VERSION_PEP440}")

if(SKBUILD)
  # AppleClang 15 changed linking order
  # https://developer.apple.com/documentation/xcode-release-notes/xcode-15-release-notes#Linking
  if(APPLE AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15.0)
    message(STATUS "Building on macOS with Clang 15 or newer")
    add_link_options("-ld_classic")
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE
  )
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Custom install target for docs to depend on.
add_custom_target(
  install-scipp COMMAND cmake --build ${CMAKE_CURRENT_BINARY_DIR} --target
                        install
)

include(docs)
add_docs_target(docs BUILDER html DEPENDS install-scipp)
add_docs_target(doctest BUILDER doctest DEPENDS docs)
add_docs_target(linkcheck BUILDER linkcheck DEPENDS docs)
add_custom_target(
  cleanup_docs_html
  COMMENT "Remove unnecessary .ipynb files from built HTML documentation."
  COMMAND find "${CMAKE_BINARY_DIR}/html" -type f -name "*.ipynb" -not -path
          "${CMAKE_BINARY_DIR}/html/_sources/*" -delete
  VERBATIM
)
add_dependencies(cleanup_docs_html docs)

include(GNUInstallDirs)
if(DEFINED ENV{SP_DIR}) # Defined by conda-build
  file(TO_CMAKE_PATH $ENV{SP_DIR}/scipp PYTHONDIR)
  file(TO_CMAKE_PATH $ENV{SP_DIR} ARCHIVEDIR)
else()
  set(PYTHONDIR scipp)
  set(ARCHIVEDIR .)
endif()
set(INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})

if(SKBUILD)
  set(PYTHONDIR .)
  # scikit-build will not include files outside the directory of the Python
  # module, ensure our shared objects are installed there.
  set(RELATIVE_LIB ".")
  # Note that we do not install src/scipp when making a `pip` package. It will
  # simply use the source directory directly. C++ libraries get installed into
  # the source dir.
else()
  install(DIRECTORY "src/scipp/" DESTINATION ${PYTHONDIR})
  set(RELATIVE_LIB "..")
endif()

enable_testing()

add_subdirectory(lib)
