# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
set(INC_FILES unit.h)

set(SRC_FILES unit.cpp)

add_library(ScippUnits STATIC ${INC_FILES} ${SRC_FILES})
target_link_libraries(ScippUnits PUBLIC Boost::boost)
target_include_directories(ScippUnits PUBLIC .)
set_target_properties(ScippUnits PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

add_subdirectory(test)
