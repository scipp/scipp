// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/variable/variable_comparison.h"

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

TEST(ComparisonTest, less_variances_test) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0},
                                     Variances{1.0, 1.0});
  const auto b = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{0.0, 3.0},
                                     Variances{1.0, 1.0});
  EXPECT_THROW(is_less(a, b), std::runtime_error);
}

TEST(ComparisonTest, less_dtypes_test) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  const auto b = makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{0, 1});
  EXPECT_THROW(is_less(a, b), std::runtime_error);
}

TEST(ComparisonTest, less_units_test) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.0, 3.0});
  b.setUnit(units::m);
  EXPECT_THROW(is_less(a, b), std::runtime_error);
}


template <typename T> class LessTest : public ::testing::Test {};
using LessTestTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(LessTest, LessTestTypes);

TYPED_TEST(LessTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  const auto a = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, x});
  const auto b = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, -x});
  const auto result1 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, false});
  const auto result2 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true});

  EXPECT_EQ(is_less(a, b), result1);
  EXPECT_EQ(is_less(b, a), result2);
}
