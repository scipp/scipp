# ~~~
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-common")
set(INC_FILES
    include/scipp/common/index.h include/scipp/common/index_composition.h
    include/scipp/common/overloaded.h include/scipp/common/reduction.h
    include/scipp/common/span.h include/scipp/common/initialization.h
)

add_library(${TARGET_NAME} INTERFACE)
set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME common)

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
)

add_subdirectory(test)

scipp_install_component(TARGET ${TARGET_NAME} NO_EXPORT_HEADER)
