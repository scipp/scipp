# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-dataset-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(
  ${TARGET_NAME}
  astype_test.cpp
  binned_arithmetic_test.cpp
  binned_creation_test.cpp
  bins_test.cpp
  bins_reduction_test.cpp
  bins_view_test.cpp
  bin_test.cpp
  concat_test.cpp
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
  equals_nan_test.cpp
  event_data_operations_consistency_test.cpp
  except_test.cpp
  generated_test.cpp
  groupby_test.cpp
  histogram_test.cpp
  logical_reduction_test.cpp
  masks_test.cpp
  mean_test.cpp
  merge_test.cpp
  minmax_test.cpp
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
target_link_libraries(
  ${TARGET_NAME} LINK_PRIVATE scipp-dataset scipp_test_helpers
  GTest::gtest_main
)

if(WIN32)
  target_compile_options(${TARGET_NAME} PRIVATE "/bigobj")
endif()

set_property(
  TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION
                                 ${INTERPROCEDURAL_OPTIMIZATION_TESTS}
)
set_property(
  TARGET ${TARGET_NAME} PROPERTY EXCLUDE_FROM_ALL $<NOT:$<BOOL:${FULL_BUILD}>>
)
if(SANITIZERS)
  add_sanitizers(${TARGET_NAME})
endif()
scipp_test(${TARGET_NAME} dataset)
