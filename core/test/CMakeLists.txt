# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-core-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL
               bool_test.cpp
               concatenate_test.cpp
               coords_proxy_test.cpp
               counts_test.cpp
               data_array_comparison_test.cpp
               data_array_test.cpp
               data_proxy_test.cpp
               dataset_comparison_test.cpp
               dataset_binary_arithmetic_test.cpp
               dataset_operations_test.cpp
               dataset_proxy_test.cpp
               dataset_test_common.cpp
               dataset_test.cpp
               dimensions_test.cpp
               except_test.cpp
               groupby_test.cpp
               histogram_test.cpp
               indexed_slice_view_test.cpp
               mean_test.cpp
               merge_test.cpp
               rebin_test.cpp
               slice_test.cpp
               sort_test.cpp
               sparse_counts_test.cpp
               transform_test.cpp
               value_and_variance_test.cpp
               variable_custom_type_test.cpp
               variable_operations_test.cpp
               variable_scalar_accessors_test.cpp
               variable_test.cpp
               variable_trigonometry_test.cpp
               variable_view_test.cpp
               view_index_test.cpp
               )
include_directories(SYSTEM ${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR})
target_link_libraries(${TARGET_NAME}
                      LINK_PRIVATE
                      scipp-core
                      gtest_main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ${INTERPROCEDURAL_OPTIMIZATION_TESTS})
add_sanitizers(${TARGET_NAME})
if(${WITH_CTEST})
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/core/)
endif()
