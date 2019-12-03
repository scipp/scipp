// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/comparison.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::units;

TEST(ComparisonTest, variable_equal) {
  const auto a =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_TRUE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_float_equal) {
  const auto a =
      createVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b =
      createVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_TRUE(is_approx(a, b, 0.1f));
}

TEST(ComparisonTest, variable_not_equal_within_tolerance) {
  const auto a =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.1});
  EXPECT_TRUE(is_approx(a, b, 0.2));
}

TEST(ComparisonTest, variable_not_equal_outside_tolerance) {
  const auto a =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.1});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_equal) {
  const auto a = createVariable<double>(Values{10.0}, Variances{1.0});
  const auto b = createVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_TRUE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_not_equal_outside_tolerance) {
  const auto a = createVariable<double>(Values{10.0}, Variances{1.0});
  const auto b = createVariable<double>(Values{10.0}, Variances{0.5});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_missing_in_one_operand) {
  const auto a = createVariable<double>(Values{10.0});
  const auto b = createVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_unit_not_equal) {
  const auto a =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  auto b = createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  b.setUnit(units::m);
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_mismatched_dtype) {
  const auto a =
      createVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b =
      createVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}
