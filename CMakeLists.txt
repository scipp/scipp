# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
cmake_minimum_required(VERSION 3.21)

cmake_policy(SET CMP0091 NEW)

execute_process(
  COMMAND git describe --tags --abbrev=0 --exclude nightly
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
