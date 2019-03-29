# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
add_executable(scipp_units_test unit_test.cpp)
target_link_libraries(scipp_units_test
                      LINK_PRIVATE
                      ScippUnits
                      gtest_main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
