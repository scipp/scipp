# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
add_executable(scipp-core-test
               bool_test.cpp
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
add_sanitizers(scipp-core-test)
target_link_libraries(scipp-core-test
                      LINK_PRIVATE
                      scipp-core
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
target_include_directories(scipp-core-test SYSTEM
                           PRIVATE "../../range-v3/include")
gtest_discover_tests(scipp-core-test TEST_PREFIX scipp/core/)
