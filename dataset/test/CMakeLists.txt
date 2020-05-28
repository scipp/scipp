# SPDX-License-Identifier: GPL-3.0-or-later Copyright (c) 2019 Scipp
# contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-dataset-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(
  ${TARGET_NAME} EXCLUDE_FROM_ALL
  attributes_test.cpp
  choose_test.cpp
  concatenate_test.cpp
  coords_view_test.cpp
  copy_test.cpp
  counts_test.cpp
  data_array_comparison_test.cpp
  data_array_test.cpp
  dataset_arithmetic_test.cpp
  dataset_comparison_test.cpp
  dataset_operations_test.cpp
  dataset_test_common.cpp
  dataset_test.cpp
  dataset_view_test.cpp
  data_view_test.cpp
  event_test.cpp
  except_test.cpp
  groupby_test.cpp
  histogram_test.cpp
  math_test.cpp
  masks_view_test.cpp
  mean_test.cpp
  merge_test.cpp
  rebin_test.cpp
  reduce_events_test.cpp
  self_assignment_test.cpp
  slice_test.cpp
  sort_test.cpp
  event_data_operations_consistency_test.cpp
  string_test.cpp
  sum_test.cpp
  transform_events_and_dense_test.cpp
  unaligned_test.cpp
)
include_directories(SYSTEM ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
target_link_libraries(
  ${TARGET_NAME}
  LINK_PRIVATE
  scipp-dataset
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
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/dataset/)
endif()
