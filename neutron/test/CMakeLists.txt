# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-neutron-test")
add_executable(${TARGET_NAME}
               #EventWorkspace_test.cpp
               #example_instrument_test.cpp
               #Run_test.cpp
               #TableWorkspace_test.cpp
               #Workspace2D_test.cpp
               beamline_test.cpp
               convert_test.cpp
               )
target_link_libraries(${TARGET_NAME}
                      LINK_PRIVATE
                      scipp-neutron
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ${INTERPROCEDURAL_OPTIMIZATION_TESTS})
add_sanitizers(${TARGET_NAME})
if(${WITH_CTEST})
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/neutron/)
endif()
