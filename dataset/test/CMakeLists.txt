# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-dataset-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(
  ${TARGET_NAME} EXCLUDE_FROM_ALL
  astype_test.cpp
  attributes_test.cpp
  binned_arithmetic_test.cpp
  binned_creation_test.cpp
  bins_test.cpp
  bin_test.cpp
  choose_test.cpp
  concatenate_test.cpp
  coords_view_test.cpp
  copy_test.cpp
  counts_test.cpp
  data_array_arithmetic_test.cpp
  data_array_comparison_test.cpp
  data_array_test.cpp
  dataset_arithmetic_test.cpp
  dataset_comparison_test.cpp
  dataset_operations_test.cpp
  dataset_test_common.cpp
  dataset_test.cpp
  dataset_view_test.cpp
  data_view_test.cpp
  event_data_operations_consistency_test.cpp
  except_test.cpp
  generated_test.cpp
  groupby_test.cpp
  histogram_test.cpp
  masks_test.cpp
  mean_test.cpp
  merge_test.cpp
  rebin_test.cpp
  self_assignment_test.cpp
  set_slice_test.cpp
  shape_test.cpp
  size_of_test.cpp
  slice_by_value_test.cpp
  slice_test.cpp
  sort_test.cpp
  string_test.cpp
  test_data_arrays.cpp
  sum_test.cpp
  to_unit_test.cpp
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
