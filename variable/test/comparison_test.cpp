// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "fix_typed_test_suite_warnings.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::units;

template <typename T> class IsApproxTest : public ::testing::Test {};
using TestTypes = ::testing::Types<double, float, int64_t, int32_t>;

TYPED_TEST_SUITE(IsApproxTest, TestTypes);

TYPED_TEST(IsApproxTest, when_variable_equal) {
  const auto a = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(is_approx(a, a, makeVariable<TypeParam>(Values{1})),
            true * units::one);
}

TYPED_TEST(IsApproxTest, when_variables_within_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{0});
  const auto b = makeVariable<TypeParam>(Values{1});
  EXPECT_EQ(is_approx(a, b, makeVariable<TypeParam>(Values{1})),
            true * units::one);
}
TYPED_TEST(IsApproxTest, when_variables_outside_tolerance) {
  const auto a = makeVariable<TypeParam>(Values{0});
  const auto b = makeVariable<TypeParam>(Values{2});
  EXPECT_EQ(is_approx(a, b, makeVariable<TypeParam>(Values{1})),
            false * units::one);
}

TEST(IsApproxTest, variances_ignored) {
  const auto a = makeVariable<double>(Values{10.0}, Variances{1.0});
  EXPECT_TRUE(a.hasVariances());
  auto out = is_approx(a, a, makeVariable<double>(Values{1}));
  EXPECT_FALSE(out.hasVariances());
}

TEST(IsApproxTest, variable_units_not_same) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  a.setUnit(units::dimensionless);
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  b.setUnit(units::m);
  auto t = makeVariable<double>(Values{0.1});
  t.setUnit(a.unit());
  // second and first argument have different units
  EXPECT_THROW(is_approx(a, b, t), except::MismatchError<scipp::units::Unit>);
  b.setUnit(a.unit());
  t.setUnit(units::m);
  // tolerance has difference units
  EXPECT_THROW(is_approx(a, b, t), except::MismatchError<scipp::units::Unit>);
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
