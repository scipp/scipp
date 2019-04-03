# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_executable(scipp_core_test
               bool_test.cpp
               convert_test.cpp
               dataset_test.cpp
               dimensions_test.cpp
               EventWorkspace_test.cpp
               example_instrument_test.cpp
               except_test.cpp
               md_zip_view_test.cpp
               multi_index_test.cpp
               Run_test.cpp
               TableWorkspace_test.cpp
               tags_test.cpp
               variable_test.cpp
               variable_view_test.cpp
               Workspace2D_test.cpp
               zip_view_test.cpp)
target_link_libraries(scipp_core_test
                      LINK_PRIVATE
                      ScippCore
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
target_include_directories(scipp_core_test SYSTEM
                           PRIVATE "../../range-v3/include")
gtest_discover_tests(scipp_core_test TEST_PREFIX scipp/core/)
