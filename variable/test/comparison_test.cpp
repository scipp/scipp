// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

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

TEST(ComparisonTest, less_variances_test) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0},
                                     Variances{1.0, 1.0});
  const auto b = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{0.0, 3.0},
                                     Variances{1.0, 1.0});
  EXPECT_THROW(less(a, b), std::runtime_error);
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

template <typename T> class LessTest : public ::testing::Test {};
template <typename T> class GreaterTest : public ::testing::Test {};
template <typename T> class LessEqualTest : public ::testing::Test {};
template <typename T> class GreaterEqualTest : public ::testing::Test {};
template <typename T> class EqualTest : public ::testing::Test {};
template <typename T> class NotEqualTest : public ::testing::Test {};

using CompareTestTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(GreaterTest, CompareTestTypes);
TYPED_TEST_SUITE(LessTest, CompareTestTypes);
TYPED_TEST_SUITE(LessEqualTest, CompareTestTypes);
TYPED_TEST_SUITE(GreaterEqualTest, CompareTestTypes);
TYPED_TEST_SUITE(EqualTest, CompareTestTypes);
TYPED_TEST_SUITE(NotEqualTest, CompareTestTypes);

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

  EXPECT_EQ(less(a, b), result1);
  EXPECT_EQ(less(b, a), result2);
}

TYPED_TEST(GreaterTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  const auto a = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, x});
  const auto b = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, -x});
  const auto result1 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true});
  const auto result2 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, false});

  EXPECT_EQ(greater(a, b), result1);
  EXPECT_EQ(greater(b, a), result2);
}

TYPED_TEST(GreaterEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  const auto a = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, x});
  const auto b = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, -x});
  const auto result1 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true});
  const auto result2 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(greater_equal(a, b), result1);
  EXPECT_EQ(greater_equal(b, a), result2);
}

TYPED_TEST(LessEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  const auto a = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, x});
  const auto b = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, -x});
  const auto result1 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});
  const auto result2 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true});

  EXPECT_EQ(less_equal(a, b), result1);
  EXPECT_EQ(less_equal(b, a), result2);
}

TYPED_TEST(EqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 1;
  const auto a = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, x});
  const auto b = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, -x});
  const auto result1 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});
  const auto result2 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(equal(a, b), result1);
  EXPECT_EQ(equal(b, a), result2);
}

TYPED_TEST(NotEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 1;
  const auto a = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, x});
  const auto b = makeVariable<T>(Dims{Dim::X}, Shape{2}, Values{y, -x});
  const auto result1 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true});
  const auto result2 =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true});

  EXPECT_EQ(not_equal(a, b), result1);
  EXPECT_EQ(not_equal(b, a), result2);
}

