// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/comparison.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::units;

TEST(ComparisonTest, variable_equal) {
  const auto a = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  const auto b = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  EXPECT_TRUE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_float_equal) {
  const auto a = makeVariable<float>({Dim::X, 2}, {1.0, 2.0});
  const auto b = makeVariable<float>({Dim::X, 2}, {1.0, 2.0});
  EXPECT_TRUE(is_approx(a, b, 0.1f));
}

TEST(ComparisonTest, variable_not_equal_within_tolerance) {
  const auto a = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  const auto b = makeVariable<double>({Dim::X, 2}, {1.1, 2.1});
  EXPECT_TRUE(is_approx(a, b, 0.2));
}

TEST(ComparisonTest, variable_not_equal_outside_tolerance) {
  const auto a = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  const auto b = makeVariable<double>({Dim::X, 2}, {1.1, 2.1});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_equal) {
  const auto a = makeVariable<double>(10.0, 1.0);
  const auto b = makeVariable<double>(10.0, 1.0);
  EXPECT_TRUE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_not_equal_outside_tolerance) {
  const auto a = makeVariable<double>(10.0, 1.0);
  const auto b = makeVariable<double>(10.0, 0.5);
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_unit_not_equal) {
  const auto a = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  auto b = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  b.setUnit(units::m);
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_mismatched_dtype) {
  const auto a = makeVariable<float>({Dim::X, 2}, {1.0, 2.0});
  const auto b = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}
