# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(INC_FILES
    include/scipp/units/simple.h
    include/scipp/units/unit.h
    include/scipp/units/unit.tcc
    include/scipp/units/neutron.h)

set(SRC_FILES neutron.cpp simple.cpp)

# For now we just enable "neutron" units by default. In the future we
# may want to build separate libraries with separate units.
if(NOT SCIPP_UNITS_SYSTEM)
  set(SCIPP_UNITS_SYSTEM "Neutron"
      CACHE STRING "Choose the unit system, options are: Neutron Simple."
      FORCE)
endif()
string(TOUPPER ${SCIPP_UNITS_SYSTEM} SCIPP_UNITS_SYSTEM)

add_library(scipp-units STATIC ${INC_FILES} ${SRC_FILES})
target_compile_definitions(scipp-units PUBLIC SCIPP_UNITS_${SCIPP_UNITS_SYSTEM})
target_link_libraries(scipp-units PUBLIC Boost::boost)
target_include_directories(
  scipp-units
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(scipp-units PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)
