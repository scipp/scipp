# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.

add_library ( Dataset STATIC dataset.cpp dataset_view.cpp dimensions.cpp unit.cpp variable.cpp except.cpp )
target_link_libraries ( Dataset PUBLIC Boost::boost OpenMP::OpenMP_CXX )
target_include_directories ( Dataset SYSTEM PUBLIC "." ${CMAKE_BINARY_DIR}/gsl-src/include PRIVATE "../range-v3/include" )
set_target_properties ( Dataset PROPERTIES POSITION_INDEPENDENT_CODE TRUE )
