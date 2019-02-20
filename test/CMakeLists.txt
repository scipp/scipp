# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_executable ( dataset_test tags_test.cpp dataset_test.cpp md_zip_view_test.cpp variable_test.cpp variable_view_test.cpp dimensions_test.cpp unit_test.cpp multi_index_test.cpp TableWorkspace_test.cpp Workspace2D_test.cpp EventWorkspace_test.cpp zip_view_test.cpp Run_test.cpp except_test.cpp example_instrument_test.cpp bool_test.cpp )
target_link_libraries( dataset_test
  LINK_PRIVATE
  Dataset
  gtest_main
  ${GTEST_LIBRARIES} ${GMOCK_LIBRARIES} )
target_include_directories ( dataset_test SYSTEM PRIVATE "../range-v3/include" )
