# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-units-test")
set(TEST_TARGETS "${TEST_TARGETS} ${TARGET_NAME}" CACHE INTERNAL "")
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL dimension_test.cpp unit_test.cpp)
target_link_libraries(${TARGET_NAME}
                      LINK_PRIVATE
                      scipp-units
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ${INTERPROCEDURAL_OPTIMIZATION_TESTS})
add_sanitizers(${TARGET_NAME})
if(${WITH_CTEST})
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/units/)
endif()
