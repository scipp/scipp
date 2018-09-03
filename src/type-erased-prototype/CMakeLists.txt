# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_subdirectory ( test )
add_subdirectory ( benchmark )

add_library ( Dataset STATIC dataset.cpp dataset_view.cpp dimensions.cpp unit.cpp variable.cpp )
target_include_directories ( Dataset PUBLIC "." ${CMAKE_BINARY_DIR}/gsl-src/include )
