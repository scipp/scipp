// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::units;

TEST(ComparisonTest, variable_equal) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_TRUE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_float_equal) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_TRUE(is_approx(a, b, 0.1f));
}

TEST(ComparisonTest, variable_not_equal_within_tolerance) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.1});
  EXPECT_TRUE(is_approx(a, b, 0.2));
}

TEST(ComparisonTest, variable_not_equal_outside_tolerance) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.1});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_equal) {
  const auto a = makeVariable<double>(Values{10.0}, Variances{1.0});
  const auto b = makeVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_TRUE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_not_equal_outside_tolerance) {
  const auto a = makeVariable<double>(Values{10.0}, Variances{1.0});
  const auto b = makeVariable<double>(Values{10.0}, Variances{0.5});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_variances_missing_in_one_operand) {
  const auto a = makeVariable<double>(Values{10.0});
  const auto b = makeVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_unit_not_equal) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  b.setUnit(units::m);
  EXPECT_FALSE(is_approx(a, b, 0.1));
}

TEST(ComparisonTest, variable_mismatched_dtype) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_THROW(is_approx(a, b, 0.1), except::TypeError);
}

TEST(ComparisonTest, tolerance_type_mismatch) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_THROW(is_approx(a, b, 0 /*implicit int*/), except::TypeError);
  EXPECT_NO_THROW(is_approx(a, b, 0.0f));
}

TEST(ComparisonTest, variances_test) {
  const auto a = makeVariable<float>(Values{1.0}, Variances{1.0});
  const auto b = makeVariable<float>(Values{2.0}, Variances{2.0});
  EXPECT_EQ(less(a, b), true * units::one);
  EXPECT_EQ(less_equal(a, b), true * units::one);
  EXPECT_EQ(greater(a, b), false * units::one);
  EXPECT_EQ(greater_equal(a, b), false * units::one);
  EXPECT_EQ(equal(a, b), false * units::one);
  EXPECT_EQ(not_equal(a, b), true * units::one);
}

TEST(ComparisonTest, less_dtypes_test) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{0, 1});
  EXPECT_THROW(less(a, b), std::runtime_error);
}

TEST(ComparisonTest, less_units_test) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.0, 3.0});
  b.setUnit(units::m);
  EXPECT_THROW(less(a, b), std::runtime_error);
}

namespace {
const auto a = 1.0 * units::m;
const auto b = 2.0 * units::m;
const auto true_ = true * units::one;
const auto false_ = false * units::one;
TEST(ComparisonTest, less_test) {
  EXPECT_EQ(less(a, b), true_);
  EXPECT_EQ(less(b, a), false_);
  EXPECT_EQ(less(a, a), false_);
}
TEST(ComparisonTest, greater_test) {
  EXPECT_EQ(greater(a, b), false_);
  EXPECT_EQ(greater(b, a), true_);
  EXPECT_EQ(greater(a, a), false_);
}
TEST(ComparisonTest, greater_equal_test) {
  EXPECT_EQ(greater_equal(a, b), false_);
  EXPECT_EQ(greater_equal(b, a), true_);
  EXPECT_EQ(greater_equal(a, a), true_);
}
TEST(ComparisonTest, less_equal_test) {
  EXPECT_EQ(less_equal(a, b), true_);
  EXPECT_EQ(less_equal(b, a), false_);
  EXPECT_EQ(less_equal(a, a), true_);
}
TEST(ComparisonTest, equal_test) {
  EXPECT_EQ(equal(a, b), false_);
  EXPECT_EQ(equal(b, a), false_);
  EXPECT_EQ(equal(a, a), true_);
}
TEST(ComparisonTest, not_equal_test) {
  EXPECT_EQ(not_equal(a, b), true_);
  EXPECT_EQ(not_equal(b, a), true_);
  EXPECT_EQ(not_equal(a, a), false_);
}
} // namespace
