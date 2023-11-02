// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"
#include <type_traits>

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"

using namespace scipp;

TEST(CommonTypeTest, arithmetic_types) {
  EXPECT_EQ(common_type(dtype<int32_t>, dtype<int32_t>),
            (dtype<std::common_type_t<int32_t, int32_t>>));
  EXPECT_EQ(common_type(dtype<int32_t>, dtype<int64_t>),
            (dtype<std::common_type_t<int32_t, int64_t>>));
  EXPECT_EQ(common_type(dtype<int32_t>, dtype<float>),
            (dtype<std::common_type_t<int32_t, float>>));
  EXPECT_EQ(common_type(dtype<int32_t>, dtype<double>),
            (dtype<std::common_type_t<int32_t, double>>));

  EXPECT_EQ(common_type(dtype<int64_t>, dtype<int32_t>),
            (dtype<std::common_type_t<int64_t, int32_t>>));
  EXPECT_EQ(common_type(dtype<int64_t>, dtype<int64_t>),
            (dtype<std::common_type_t<int64_t, int64_t>>));
  EXPECT_EQ(common_type(dtype<int64_t>, dtype<float>),
            (dtype<std::common_type_t<int64_t, float>>));
  EXPECT_EQ(common_type(dtype<int64_t>, dtype<double>),
            (dtype<std::common_type_t<int64_t, double>>));

  EXPECT_EQ(common_type(dtype<float>, dtype<int32_t>),
            (dtype<std::common_type_t<float, int32_t>>));
  EXPECT_EQ(common_type(dtype<float>, dtype<int64_t>),
            (dtype<std::common_type_t<float, int64_t>>));
  EXPECT_EQ(common_type(dtype<float>, dtype<float>),
            (dtype<std::common_type_t<float, float>>));
  EXPECT_EQ(common_type(dtype<float>, dtype<double>),
            (dtype<std::common_type_t<float, double>>));

  EXPECT_EQ(common_type(dtype<double>, dtype<int32_t>),
            (dtype<std::common_type_t<double, int32_t>>));
  EXPECT_EQ(common_type(dtype<double>, dtype<int64_t>),
            (dtype<std::common_type_t<double, int64_t>>));
  EXPECT_EQ(common_type(dtype<double>, dtype<float>),
            (dtype<std::common_type_t<double, float>>));
  EXPECT_EQ(common_type(dtype<double>, dtype<double>),
            (dtype<std::common_type_t<double, double>>));
}

TEST(CommonTypeTest, same_non_arithmetic_type) {
  EXPECT_EQ(common_type(dtype<core::time_point>, dtype<core::time_point>),
            dtype<core::time_point>);
}

// NOTE: Exceptions not tested here, since DType name registry is initialized
// only later, in scipp::variable. See tests in variable/test/astype_test.cpp
