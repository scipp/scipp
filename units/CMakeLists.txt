# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-units")
set(INC_FILES include/scipp/units/except.h include/scipp/units/string.h
              include/scipp/units/unit.h include/scipp/units/neutron.h
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
target_link_libraries(${TARGET_NAME} PUBLIC scipp-common Boost::boost)
target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         ${CMAKE_CURRENT_BINARY_DIR}
)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)

if(DYNAMIC_LIB)
  install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif(DYNAMIC_LIB)
