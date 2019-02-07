# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
set(INC_FILES
convert.h dataset.h md_zip_view.h dimensions.h unit.h variable.h except.h)

set(SRC_FILES
convert.cpp dataset.cpp md_zip_view.cpp dimensions.cpp unit.cpp variable.cpp except.cpp)

add_library ( Dataset STATIC  ${INC_FILES} ${SRC_FILES})
target_link_libraries ( Dataset PUBLIC Boost::boost OpenMP::OpenMP_CXX )
target_include_directories ( Dataset SYSTEM PUBLIC "." ${CMAKE_BINARY_DIR}/gsl-src/include ${CMAKE_BINARY_DIR}/Eigen-src PRIVATE "../range-v3/include" )
set_target_properties ( Dataset PROPERTIES POSITION_INDEPENDENT_CODE TRUE )
