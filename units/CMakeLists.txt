# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(INC_FILES
    include/scipp/units/dummy.h
    include/scipp/units/unit.h
    include/scipp/units/unit.tcc
    include/scipp/units/neutron.h)

set(SRC_FILES dummy.cpp neutron.cpp)

# For now we just enable "neutron" units by default. In the future we
# may want to build separate libraries with separate units.
if(NOT SCIPP_UNITS_SYSTEM)
  set(SCIPP_UNITS_SYSTEM "Neutron"
      CACHE STRING "Choose the unit system, options are: Dummy Neutron."
      FORCE)
endif()
string(TOUPPER ${SCIPP_UNITS_SYSTEM} SCIPP_UNITS_SYSTEM)

add_library(scipp-units SHARED ${INC_FILES} ${SRC_FILES})
target_compile_definitions(scipp-units PUBLIC SCIPP_UNITS_${SCIPP_UNITS_SYSTEM})
target_link_libraries(scipp-units PUBLIC Boost::boost)
target_include_directories(
  scipp-units
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(scipp-units PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
install(TARGETS scipp-units LIBRARY DESTINATION lib)

add_subdirectory(test)
