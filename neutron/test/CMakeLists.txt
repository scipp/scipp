# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
add_executable(scipp-neutron-test
               #EventWorkspace_test.cpp
               #example_instrument_test.cpp
               #Run_test.cpp
               #TableWorkspace_test.cpp
               #Workspace2D_test.cpp
               #convert_test.cpp
               )
target_link_libraries(scipp-neutron-test
                      LINK_PRIVATE
                      scipp-neutron
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
add_sanitizers(scipp-neutron-test)
if(${WITH_CTEST})
  gtest_discover_tests(scipp-neutron-test TEST_PREFIX scipp/neutron/)
endif()
