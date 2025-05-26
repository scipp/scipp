// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <limits>
#include <tuple>

#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/pow.h"

using namespace scipp;

TEST(ArithmeticTest, x_plus_x_with_variances_equals_2_x) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  const auto two = makeVariable<double>(Values{2.0});
  EXPECT_EQ(x + x, two * x);
}

TEST(ArithmeticTest, x_plus_x_with_variances_and_no_unit_equals_2_x) {
  const auto x =
      makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::none);
  const auto two = makeVariable<double>(Values{2.0}, sc_units::none);
  EXPECT_EQ(x + x, two * x);
}

TEST(ArithmeticTest,
     x_plus_shallow_copy_of_x_with_variances_handles_correlations) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  EXPECT_EQ(x + Variable(x), x + x);
}

TEST(ArithmeticTest,
     x_plus_copy_of_x_with_variances_does_not_handle_correlations) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  // x and copy(x) are NOT detected as correlated
  EXPECT_NE(x + copy(x), x + x);
}

TEST(ArithmeticTest, slice_of_x_plus_slice_of_x_handles_correlations) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0},
                                      Variances{4.0, 3.0}, sc_units::m);
  const auto two = makeVariable<double>(Values{2.0});
  EXPECT_EQ(x.slice({Dim::X, 0}) + x.slice({Dim::X, 0}),
            two * x.slice({Dim::X, 0}));
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

TEST(ArithmeticTest, x_minus_x_inf_with_variances_equals_nan) {
  const auto x = makeVariable<double>(
      Values{std::numeric_limits<double>::infinity()}, Variances{4.0});
  EXPECT_TRUE(
      isclose(
          x - x,
          makeVariable<double>(Values{std::numeric_limits<double>::quiet_NaN()},
                               Variances{0.0}),
          makeVariable<double>(Values{1.0}), makeVariable<double>(Values{0.0}),
          variable::NanComparisons::Equal)
          .value<bool>());
}

TEST(ArithmeticTest, x_minus_equals_x_with_variances_equals_0_x) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0});
  const auto zero = makeVariable<double>(Values{0.0});
  const auto expected = zero * x;
  EXPECT_EQ(x -= x, expected);
  EXPECT_EQ(x, expected);
}

TEST(ArithmeticTest, x_times_x_with_variances_equals_x_squared) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  const auto two = makeVariable<double>(Values{2.0});
  EXPECT_EQ(x * x, pow(x, two));
}

TEST(ArithmeticTest, x_times_equals_x_with_variances_equals_x_squared) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  const auto two = makeVariable<double>(Values{2.0});
  const auto expected = pow(x, two);
  EXPECT_EQ(x *= x, expected);
  EXPECT_EQ(x, expected);
}

TEST(ArithmeticTest, x_divide_x_with_variances_equals_x_to_the_power_of_zero) {
  const auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  const auto zero = makeVariable<double>(Values{0.0});
  EXPECT_EQ(x / x, pow(x, zero));
}

TEST(ArithmeticTest,
     x_divide_equals_x_with_variances_equals_x_to_the_power_of_zero) {
  auto x = makeVariable<double>(Values{2.0}, Variances{4.0}, sc_units::m);
  const auto zero = makeVariable<double>(Values{0.0});
  const auto expected = pow(x, zero);
  EXPECT_EQ(x /= x, expected);
  EXPECT_EQ(x, expected);
}

TEST(ArithmeticTest, binned_x_plus_x_with_variances_equals_2_x) {
  const auto indices = makeVariable<scipp::index_pair>(Values{std::pair{0, 1}});
  const auto buffer = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{2.0},
                                           Variances{4.0}, sc_units::m);
  const auto x = make_bins(indices, Dim::X, buffer);
  const auto two = makeVariable<double>(Values{2.0});
  EXPECT_EQ(x + x, two * x);
}
