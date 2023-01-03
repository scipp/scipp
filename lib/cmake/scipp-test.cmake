# ~~~
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# ~~~
function(scipp_test target prefix)
  if(${CTEST_DISCOVER_TESTS})
    gtest_discover_tests(${target} TEST_PREFIX scipp/${prefix}/)
  else()
    add_test(NAME ${target} COMMAND ${target} --gtest_brief=1)
  endif()
endfunction()
