# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
set(TARGET_NAME "scipp-units-test")
add_dependencies(all-tests ${TARGET_NAME})
add_executable(${TARGET_NAME} dim_test.cpp unit_test.cpp)
target_link_libraries(
  ${TARGET_NAME} LINK_PRIVATE scipp-units scipp_test_helpers GTest::gtest_main
)
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
scipp_test(${TARGET_NAME} units)
