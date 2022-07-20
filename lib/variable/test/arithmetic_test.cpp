// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <tuple>

#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/pow.h"

using namespace scipp;

TEST(ArithmeticTest, x_plus_x_with_variances_equals_2_x) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, units::m);
  const auto two = makeVariable<double>(Values{2.0});
  EXPECT_EQ(x + x, two * x);
}

TEST(ArithmeticTest, x_plus_x_with_variances_and_no_unit_equals_2_x) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, units::none);
  const auto two = makeVariable<double>(Values{2.0}, units::none);
  EXPECT_EQ(x + x, two * x);
}

TEST(ArithmeticTest, x_plus_equals_x_with_variances_equals_2_x) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto two = makeVariable<double>(Values{2.0});
  const auto expected = two * x;
  EXPECT_EQ(x += x, expected);
  EXPECT_EQ(x, expected);
}

TEST(ArithmeticTest, x_minus_x_with_variances_equals_0_x) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto zero = makeVariable<double>(Values{0.0});
  EXPECT_EQ(x - x, zero * x);
}

TEST(ArithmeticTest, x_minus_equals_x_with_variances_equals_0_x) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto zero = makeVariable<double>(Values{0.0});
  const auto expected = zero * x;
  EXPECT_EQ(x -= x, expected);
  EXPECT_EQ(x, expected);
}

TEST(ArithmeticTest, x_times_x_with_variances_equals_x_squared) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto two = makeVariable<double>(Values{2.0});
  EXPECT_EQ(x * x, pow(x, two));
}

TEST(ArithmeticTest, x_times_equals_x_with_variances_equals_x_squared) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto two = makeVariable<double>(Values{2.0});
  const auto expected = pow(x, two);
  EXPECT_EQ(x *= x, expected);
  EXPECT_EQ(x, expected);
}

TEST(ArithmeticTest, x_divide_x_with_variances_equals_x_to_the_power_of_zero) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto zero = makeVariable<double>(Values{0.0});
  EXPECT_EQ(x / x, pow(x, zero));
}

TEST(ArithmeticTest,
     x_divide_equals_x_with_variances_equals_x_to_the_power_of_zero) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto zero = makeVariable<double>(Values{0.0});
  const auto expected = pow(x, zero);
  EXPECT_EQ(x /= x, expected);
  EXPECT_EQ(x, expected);
}
