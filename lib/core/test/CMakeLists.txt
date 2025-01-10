# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-core-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(
  ${TARGET_NAME}
  array_to_string_test.cpp
  dict_test.cpp
  dimensions_test.cpp
  dtype_test.cpp
  eigen_test.cpp
  element_array_test.cpp
  element_array_view_test.cpp
  element_arithmetic_test.cpp
  element_bin_detail_test.cpp
  element_comparison_test.cpp
  element_event_operations_test.cpp
  element_geometric_operations_test.cpp
  element_histogram_test.cpp
  element_hyperbolic_test.cpp
  element_logical_test.cpp
  element_map_to_bins_test.cpp
  element_math_test.cpp
  element_special_values_test.cpp
  element_to_unit_test.cpp
  element_trigonometry_test.cpp
  element_util_test.cpp
  multi_index_test.cpp
  slice_test.cpp
  sizes_test.cpp
  spatial_transforms_test.cpp
  strides_test.cpp
  string_test.cpp
  subbin_sizes_test.cpp
  time_point_test.cpp
  value_and_variance_test.cpp
  view_index_test.cpp
  transform_common_test.cpp
)
target_link_libraries(
  ${TARGET_NAME} LINK_PRIVATE scipp-core scipp_test_helpers GTest::gtest_main
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
scipp_test(${TARGET_NAME} core)
