# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
add_executable(scipp-units-test dimension_test.cpp unit_test.cpp)
target_link_libraries(scipp-units-test
                      LINK_PRIVATE
                      scipp-units
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
add_sanitizers(scipp-units-test)
gtest_discover_tests(scipp-units-test TEST_PREFIX scipp/units/)
