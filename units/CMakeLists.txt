# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-units")
set(INC_FILES
    include/scipp/units/dummy.h
    include/scipp/units/except.h
    include/scipp/units/unit.h
    include/scipp/units/unit.tcc
    include/scipp/units/neutron.h)

set(SRC_FILES dummy.cpp except.cpp neutron.cpp)

# For now we just enable "neutron" units by default. In the future we
# may want to build separate libraries with separate units.
if(NOT SCIPP_UNITS_SYSTEM)
  set(SCIPP_UNITS_SYSTEM "Neutron"
      CACHE STRING "Choose the unit system, options are: Dummy Neutron."
      FORCE)
endif()
string(TOUPPER ${SCIPP_UNITS_SYSTEM} SCIPP_UNITS_SYSTEM)

set(LINK_TYPE "STATIC")
if(DYNAMIC_LIB)
  set(LINK_TYPE "SHARED")
endif(DYNAMIC_LIB)

add_library(${TARGET_NAME} ${LINK_TYPE} ${INC_FILES} ${SRC_FILES})
generate_export_header(${TARGET_NAME})
target_compile_definitions(${TARGET_NAME} PUBLIC
                           SCIPP_UNITS_${SCIPP_UNITS_SYSTEM})
include_directories(SYSTEM ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME} PUBLIC scipp-common Boost::boost)
target_include_directories(
  ${TARGET_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)

if(DYNAMIC_LIB)
  install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif(DYNAMIC_LIB)
