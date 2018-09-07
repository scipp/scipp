# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_executable ( dataset_test dataset_test.cpp dataset_view_test.cpp variable_test.cpp dimensions_test.cpp unit_test.cpp multi_index_test.cpp TableWorkspace_test.cpp Workspace2D_test.cpp EventWorkspace_test.cpp )
target_link_libraries( dataset_test
  LINK_PRIVATE
  Dataset
  gtest_main
  ${GTEST_LIBRARIES} )
target_include_directories ( dataset_test PRIVATE "../range-v3/include" )
