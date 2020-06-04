# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-core-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(
  ${TARGET_NAME} EXCLUDE_FROM_ALL
  dimensions_test.cpp
  element_array_test.cpp
  element_array_view_test.cpp
  element_arithmetic_test.cpp
  element_comparison_test.cpp
  element_event_operations_test.cpp
  element_geometric_operations_test.cpp
  element_histogram_test.cpp
  element_logical_test.cpp
  element_math_test.cpp
  element_reduction_test.cpp
  element_special_values_test.cpp
  element_trigonometry_test.cpp
  element_util_test.cpp
  value_and_variance_test.cpp
  view_index_test.cpp
)
include_directories(SYSTEM ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
target_link_libraries(
  ${TARGET_NAME}
  LINK_PRIVATE
  scipp-core
  gtest_main
  scipp_test_helpers
  ${GTEST_LIBRARIES}
  ${GMOCK_LIBRARIES}
)

if(WIN32)
  target_compile_options(${TARGET_NAME} PRIVATE "/bigobj")
endif()

set_property(
  TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION
                                 ${INTERPROCEDURAL_OPTIMIZATION_TESTS}
)
add_sanitizers(${TARGET_NAME})
if(${WITH_CTEST})
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/core/)
endif()
