# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
set(INC_FILES include/scipp/units/unit.h)

set(SRC_FILES unit.cpp)

add_library(scipp-units STATIC ${INC_FILES} ${SRC_FILES})
target_link_libraries(scipp-units PUBLIC Boost::boost)
target_include_directories(
  scipp-units
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(scipp-units PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)
