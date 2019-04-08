# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_executable(scipp-units-test unit_test.cpp)
target_link_libraries(scipp-units-test
                      LINK_PRIVATE
                      scipp-units
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
add_sanitizers(scipp-units-test)
gtest_discover_tests(scipp-units-test TEST_PREFIX scipp/units/)
