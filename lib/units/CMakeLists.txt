# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-units")
set(INC_FILES include/scipp/units/except.h include/scipp/units/string.h
              include/scipp/units/unit.h
)

set(SRC_FILES dim.cpp except.cpp string.cpp unit.cpp)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
target_compile_definitions(
  ${TARGET_NAME} PUBLIC SCIPP_UNITS_${SCIPP_UNITS_SYSTEM}
)
include_directories(SYSTEM ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})

target_link_libraries(${TARGET_NAME} PUBLIC scipp-common units::units)

target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME units)

if(PRECOMPILED_HEADERS)
  target_precompile_headers(${TARGET_NAME} PUBLIC ${INC_FILES})
endif()

add_subdirectory(test)

scipp_install_component(TARGET ${TARGET_NAME})

if(COVERAGE)
  append_coverage_compiler_flags()
endif()
