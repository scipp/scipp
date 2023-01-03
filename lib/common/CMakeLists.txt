# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-common")
set(INC_FILES
    include/scipp/common/index.h include/scipp/common/index_composition.h
    include/scipp/common/overloaded.h include/scipp/common/reduction.h
    include/scipp/common/initialization.h
)

add_library(${TARGET_NAME} INTERFACE)
set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME common)

target_include_directories(
  ${TARGET_NAME}
  INTERFACE $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

add_subdirectory(test)

scipp_install_component(TARGET ${TARGET_NAME} NO_EXPORT_HEADER)

if(COVERAGE)
  append_coverage_compiler_flags()
endif()
