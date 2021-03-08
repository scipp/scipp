// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/comparison.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core::element;

template <typename T> class ElementLessTest : public ::testing::Test {};
template <typename T> class ElementGreaterTest : public ::testing::Test {};
template <typename T> class ElementLessEqualTest : public ::testing::Test {};
template <typename T> class ElementGreaterEqualTest : public ::testing::Test {};
template <typename T> class ElementEqualTest : public ::testing::Test {};
template <typename T> class ElementNotEqualTest : public ::testing::Test {};
using ElementLessTestTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(ElementLessTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementGreaterTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementLessEqualTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementGreaterEqualTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementEqualTest, ElementLessTestTypes);
TYPED_TEST_SUITE(ElementNotEqualTest, ElementLessTestTypes);

TEST(ElementComparisonTest, unit) {
  const units::Unit m(units::m);
  EXPECT_EQ(comparison(m, m), units::dimensionless);
  const units::Unit rad(units::rad);
  EXPECT_THROW(comparison(rad, m), except::UnitError);
}

TYPED_TEST(ElementLessTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(less(y, x), true);
  x = -1;
  EXPECT_EQ(less(y, x), false);
  x = 1;
  EXPECT_EQ(less(y, x), false);
}

TYPED_TEST(ElementGreaterTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(greater(y, x), false);
  x = -1;
  EXPECT_EQ(greater(y, x), true);
  x = 1;
  EXPECT_EQ(greater(y, x), false);
}

TYPED_TEST(ElementLessEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(less_equal(y, x), true);
  x = 1;
  EXPECT_EQ(less_equal(y, x), true);
  x = -1;
  EXPECT_EQ(less_equal(y, x), false);
}

TYPED_TEST(ElementGreaterEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(greater_equal(y, x), false);
  x = 1;
  EXPECT_EQ(greater_equal(y, x), true);
  x = -1;
  EXPECT_EQ(greater_equal(y, x), true);
}

TYPED_TEST(ElementEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(equal(y, x), false);
  x = 1;
  EXPECT_EQ(equal(y, x), true);
  x = -1;
  EXPECT_EQ(equal(y, x), false);
}

TYPED_TEST(ElementNotEqualTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(not_equal(y, x), true);
  x = 1;
  EXPECT_EQ(not_equal(y, x), false);
  x = -1;
  EXPECT_EQ(not_equal(y, x), true);
}

template <typename T> class ElementNanMinTest : public ::testing::Test {};
template <typename T> class ElementNanMaxTest : public ::testing::Test {};
using ElementNanMinTestTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(ElementNanMinTest, ElementNanMinTestTypes);
TYPED_TEST_SUITE(ElementNanMaxTest, ElementNanMinTestTypes);

TYPED_TEST(ElementNanMinTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  nanmin_equals(y, x);
  EXPECT_EQ(y, 1);
}

TYPED_TEST(ElementNanMinTest, value_nan) {
  using T = TypeParam;
  T y = NAN;
  T x = 2;
  nanmin_equals(y, x);
  EXPECT_EQ(y, 2);
}

TYPED_TEST(ElementNanMaxTest, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  nanmax_equals(y, x);
  EXPECT_EQ(y, 2);
}

TYPED_TEST(ElementNanMaxTest, value_nan) {
  using T = TypeParam;
  T y = 1;
  T x = NAN;
  nanmax_equals(y, x);
  EXPECT_EQ(y, 1);
}

template <typename T> class IsApproxTest : public ::testing::Test {};
using IsApproxTestTypes = ::testing::Types<double, ValueAndVariance<double>>;
TYPED_TEST_SUITE(IsApproxTest, IsApproxTestTypes);

TYPED_TEST(IsApproxTest, value) {
  TypeParam a = 1.0;
  TypeParam b = 2.1;
  EXPECT_TRUE(is_approx(a, b, 1.2));
  EXPECT_TRUE(is_approx(a, b, 1.1));
  EXPECT_FALSE(is_approx(a, b, 1.0));
}

TYPED_TEST(IsApproxTest, value_not_equal_nans) {
  EXPECT_FALSE(is_approx(TypeParam(NAN), TypeParam(NAN), 1.e9));
  EXPECT_FALSE(is_approx(TypeParam(NAN), TypeParam(1.0), 1.e9));
  EXPECT_FALSE(is_approx(TypeParam(1.0), TypeParam(NAN), 1.e9));
  EXPECT_FALSE(is_approx(TypeParam(INFINITY), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(is_approx(TypeParam(1.0), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(is_approx(TypeParam(INFINITY), TypeParam(1.0), 1.e9));
  EXPECT_FALSE(is_approx(-TypeParam(INFINITY), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(is_approx(-TypeParam(1.0), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(is_approx(-TypeParam(INFINITY), -TypeParam(1.0), 1.e9));
}

TYPED_TEST(IsApproxTest, value_equal_nans) {
  EXPECT_TRUE(is_approx_equal_nan(TypeParam(NAN), TypeParam(NAN), 1.e9));
  EXPECT_FALSE(is_approx_equal_nan(TypeParam(NAN), TypeParam(1.0), 1.e9));
  EXPECT_FALSE(is_approx_equal_nan(TypeParam(1.0), TypeParam(NAN), 1.e9));
}
TYPED_TEST(IsApproxTest, value_equal_pos_infs) {
  EXPECT_TRUE(
      is_approx_equal_nan(TypeParam(INFINITY), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(is_approx_equal_nan(TypeParam(1.0), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(is_approx_equal_nan(TypeParam(INFINITY), TypeParam(1.0), 1.e9));
}
TYPED_TEST(IsApproxTest, value_equal_neg_infs) {
  EXPECT_TRUE(
      is_approx_equal_nan(-TypeParam(INFINITY), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(
      is_approx_equal_nan(-TypeParam(1.0), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(
      is_approx_equal_nan(-TypeParam(INFINITY), -TypeParam(1.0), 1.e9));
}

TYPED_TEST(IsApproxTest, value_equal_infs_signbit) {
  EXPECT_FALSE(
      is_approx_equal_nan(-TypeParam(INFINITY), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(
      is_approx_equal_nan(TypeParam(INFINITY), -TypeParam(INFINITY), 1.e9));
}

/*
TEST(IsApproxTest, value_and_variance) {
  ValueAndVariance<double> a = {0.0, 0.0};
  // tol for variances = sqrt(2) * t / y
  EXPECT_TRUE(is_approx(a, a, 1e-8)); // value and variance within tolerance
  ValueAndVariance<double> b = {1.0, 1.0};
  EXPECT_FALSE(is_approx(a, b, 0)); // value and variance outside tolerance
  ValueAndVariance<double> c = {1.01, 0.0};
  EXPECT_FALSE(is_approx(a, c, 1.0)); // value outside tolerance variance within
  ValueAndVariance<double> d = {0.0, 1.0};
  EXPECT_FALSE(is_approx(a, d,
                         1.0 / std::sqrt(2) -
                             1e-8)); // value outside tolerance variance within
}
*/

template <class Op> void do_is_approx_units_test(Op op) {
  EXPECT_EQ(units::dimensionless, op(units::m, units::m, units::m));
  EXPECT_THROW_DISCARD(op(units::m, units::m, units::s),
                       except::UnitMismatchError);
  EXPECT_THROW_DISCARD(op(units::m, units::s, units::m),
                       except::UnitMismatchError);
  EXPECT_THROW_DISCARD(op(units::s, units::m, units::m),
                       except::UnitMismatchError);
}

TEST(IsApproxTest, units) {
  do_is_approx_units_test(is_approx);
  do_is_approx_units_test(is_approx_equal_nan);
}
