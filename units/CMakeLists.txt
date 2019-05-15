# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(INC_FILES
    include/scipp/units/simple.h
    include/scipp/units/unit.h
    include/scipp/units/unit.tcc
    include/scipp/units/neutron.h)

set(SRC_FILES neutron.cpp simple.cpp)

add_library(scipp-units STATIC ${INC_FILES} ${SRC_FILES})
target_compile_definitions(scipp-units PUBLIC SCIPP_UNITS=neutron)
target_link_libraries(scipp-units PUBLIC Boost::boost)
target_include_directories(
  scipp-units
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(scipp-units PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)
