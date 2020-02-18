# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-common")
set(INC_FILES include/scipp/common/index.h include/scipp/common/overloaded.h
              include/scipp/common/span.h
)

add_library(${TARGET_NAME} INTERFACE)

# Include tcb/span as system header to avoid compiler warnings.
target_include_directories(
  ${TARGET_NAME} SYSTEM
  INTERFACE
    $<INSTALL_INTERFACE:include/scipp/common/span/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/scipp/common/span/include>
)
target_include_directories(
  ${TARGET_NAME}
  INTERFACE $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(test)
