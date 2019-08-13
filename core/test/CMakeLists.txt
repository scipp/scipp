# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
set(TARGET_NAME "scipp-core-test")
add_executable(${TARGET_NAME}
               bool_test.cpp
               coords_proxy_test.cpp
               counts_test.cpp
               data_array_comparison_test.cpp
               data_array_test.cpp
               data_proxy_test.cpp
               dataset_comparison_test.cpp
               dataset_binary_arithmetic_test.cpp
               dataset_proxy_test.cpp
               dataset_test_common.cpp
               dataset_test.cpp
               dimensions_test.cpp
               except_test.cpp
               histogram_test.cpp
               slice_test.cpp
               transform_test.cpp
               value_and_variance_test.cpp
               variable_custom_type_test.cpp
               variable_operations_test.cpp
               variable_test.cpp
               variable_view_test.cpp
               view_index_test.cpp
               #zip_view_test.cpp
               )
target_link_libraries(${TARGET_NAME}
                      LINK_PRIVATE
                      scipp-core
                      GTest::Main
                      scipp_test_helpers
                      ${GTEST_LIBRARIES}
                      ${GMOCK_LIBRARIES})
target_include_directories(${TARGET_NAME} SYSTEM
                           PRIVATE "../../range-v3/include")
set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION ${INTERPROCEDURAL_OPTIMIZATION_TESTS})
add_sanitizers(${TARGET_NAME})
if(${WITH_CTEST})
  gtest_discover_tests(${TARGET_NAME} TEST_PREFIX scipp/core/)
endif()
