// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/common/numeric.h"

#include <gtest/gtest.h>

using namespace scipp;

TEST(NumericPowTest, value_float_exponent) {
  EXPECT_NEAR(numeric::pow(3.0, 2.0), 9.0, 1e-12);
  EXPECT_NEAR(numeric::pow(3.0, -2.0), 1.0 / 9.0, 1e-12);
  EXPECT_NEAR(numeric::pow(-3.0, 2.0), 9.0, 1e-12);
  EXPECT_NEAR(numeric::pow(-3.0, -2.0), 1.0 / 9.0, 1e-12);
  EXPECT_NEAR(numeric::pow(-3.0, 3.0), -27.0, 1e-12);
  EXPECT_NEAR(numeric::pow(-3.0, -3.0), -1.0 / 27.0, 1e-12);
  EXPECT_TRUE(numeric::isnan(numeric::pow(-3.0, 3.2)));
  EXPECT_TRUE(numeric::isnan(numeric::pow(-3, 3.2)));
  EXPECT_TRUE(numeric::isnan(numeric::pow(-3.0, -3.2)));
  EXPECT_TRUE(numeric::isnan(numeric::pow(-3, -3.2)));
}

TEST(NumericPowTest, value_integer_base_integer_exponent) {
  for (int64_t base : {-5, -3, -2, -1, 0, 1, 2, 5, 10}) {
    EXPECT_EQ(numeric::pow(base, int64_t{0}), int64_t{1});
    EXPECT_EQ(numeric::pow(base, int64_t{1}), base);
    EXPECT_EQ(numeric::pow(base, int64_t{2}), base * base);
    EXPECT_EQ(numeric::pow(base, int64_t{3}), base * base * base);
  }
  EXPECT_EQ(numeric::pow(int64_t{2}, int64_t{40}), 1099511627776);
  EXPECT_EQ(numeric::pow(int64_t{7}, int64_t{15}), 4747561509943);
  // The maximum recursion depth which still produces a number representable as
  // int64_t (except for base=1).
  EXPECT_EQ(numeric::pow(int64_t{2}, int64_t{62}), 4611686018427387904);

  // exponent < 0 is undefined behavior because the result
  // is not representable as integer.
}

TEST(NumericPowTest, value_float_base_integer_exponent) {
  for (double base : {-5, -3, -2, -1, 1, 2, 5, 10}) {
    EXPECT_NEAR(numeric::pow(base, int64_t{0}), int64_t{1}, 1e-12);
    EXPECT_NEAR(numeric::pow(base, int64_t{1}), base, 1e-12);
    EXPECT_NEAR(numeric::pow(base, int64_t{2}), base * base, 1e-12);
    EXPECT_NEAR(numeric::pow(base, int64_t{3}), base * base * base, 1e-12);
    EXPECT_NEAR(numeric::pow(base, int64_t{-1}), 1.0 / base, 1e-12);
    EXPECT_NEAR(numeric::pow(base, int64_t{-2}), 1.0 / (base * base), 1e-12);
    EXPECT_NEAR(numeric::pow(base, int64_t{-3}), 1.0 / (base * base * base),
                1e-12);
  }
  EXPECT_NEAR(numeric::pow(0.0, int64_t{0}), 1.0, 1e-16);
  EXPECT_NEAR(numeric::pow(0.0, int64_t{1}), 0.0, 1e-16);
  EXPECT_NEAR(numeric::pow(0.0, int64_t{6}), 0.0, 1e-16);
  EXPECT_TRUE(std::isinf(numeric::pow(0.0, int64_t{-1})));
  EXPECT_NEAR(numeric::pow(4.125, int64_t{13}), 100117820.6814957, 1e-12);
  EXPECT_NEAR(numeric::pow(9.247, int64_t{26}), 1.3062379536886155e+25, 1e11);
}
