# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~

# This file provides a workaround for building wheels with TBB and should be
# removed once the tbb-devel packages include cmake config files.

# "python3 -m site --user-site" works locally, but not with skbuild+cibuildwheel
# since the install location of tbb-devel appears to be different.
execute_process(
  COMMAND
    python -c
    "import skbuild, inspect, os; print(os.path.dirname(inspect.getfile(skbuild)))"
  OUTPUT_VARIABLE SITE_PACKAGES_SKBUILD COMMAND_ECHO STDOUT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
file(TO_CMAKE_PATH "${SITE_PACKAGES_SKBUILD}" SITE_PACKAGES_SKBUILD)

if(UNIX)
  set(TBB_CMAKE_DIR ${SITE_PACKAGES_SKBUILD}/../../../cmake)
else()
  set(TBB_CMAKE_DIR ${SITE_PACKAGES_SKBUILD}/../../../Library/lib/cmake)
endif()

if(UNIX)
  if(APPLE)
    set(PLATFORM_DIR macos)
  else()
    set(PLATFORM_DIR linux)
  endif()
else()
  set(PLATFORM_DIR windows)
endif()

message(STATUS "Copying TBB cmake files to ${TBB_CMAKE_DIR}")
file(MAKE_DIRECTORY ${TBB_CMAKE_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/.tbb/${PLATFORM_DIR}/TBB
     DESTINATION ${TBB_CMAKE_DIR}
)

set(TBB_DIR ${TBB_CMAKE_DIR}/TBB)
