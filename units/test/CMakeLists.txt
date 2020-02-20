# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-units-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL dim_test.cpp unit_test.cpp)
target_link_libraries(
  ${TARGET_NAME}
  LINK_PRIVATE
  scipp-units
  scipp_test_helpers
  gtest_main
  ${GTEST_LIBRARIES}
  ${GMOCK_LIBRARIES}
)
set_property(
  TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION
                                 ${INTERPROCEDURAL_OPTIMIZATION_TESTS}
)
add_sanitizers(${TARGET_NAME})
if(${WITH_CTEST})
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/units/)
endif()
