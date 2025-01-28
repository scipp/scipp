# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-variable-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(
  ${TARGET_NAME}
  accumulate_test.cpp
  arithmetic_test.cpp
  astype_test.cpp
  bin_array_model_test.cpp
  bin_util_test.cpp
  comparison_test.cpp
  concat_test.cpp
  copy_test.cpp
  creation_test.cpp
  cumulative_test.cpp
  equals_nan_test.cpp
  hyperbolic_test.cpp
  linalg_test.cpp
  math_test.cpp
  mean_test.cpp
  operations_test.cpp
  rebin_test.cpp
  reduce_logical_test.cpp
  reduce_various_test.cpp
  shape_test.cpp
  slice_test.cpp
  sort_test.cpp
  inv_test.cpp
  special_values_test.cpp
  subspan_view_test.cpp
  sum_test.cpp
  test_variables.cpp
  to_unit_test.cpp
  transform_test.cpp
  transform_binary_test.cpp
  transform_test_helpers.h
  transform_test_helpers.cpp
  transform_unary_test.cpp
  trigonometry_test.cpp
  util_test.cpp
  variable_bin_test.cpp
  variable_comparison_test.cpp
  variable_custom_type_test.cpp
  variable_keyword_args_constructor_test.cpp
  variable_scalar_accessors_test.cpp
  variable_structure_test.cpp
  variable_test.cpp
)
target_link_libraries(
  ${TARGET_NAME} LINK_PRIVATE scipp-variable scipp_test_helpers
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
scipp_test(${TARGET_NAME} variable)
