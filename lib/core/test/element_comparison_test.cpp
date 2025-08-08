// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  const sc_units::Unit m(sc_units::m);
  EXPECT_EQ(comparison(m, m), sc_units::none);
  const sc_units::Unit rad(sc_units::rad);
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

template <typename T> class IsCloseTest : public ::testing::Test {};
using IsCloseTestTypes = ::testing::Types<double, ValueAndVariance<double>>;
TYPED_TEST_SUITE(IsCloseTest, IsCloseTestTypes);

TYPED_TEST(IsCloseTest, value) {
  TypeParam a = 1.0;
  TypeParam b = 2.1;
  EXPECT_TRUE(isclose(a, b, 1.2));
  EXPECT_TRUE(isclose(a, b, 1.1));
  EXPECT_FALSE(isclose(a, b, 1.0));
}

TYPED_TEST(IsCloseTest, value_not_equal_nans) {
  EXPECT_FALSE(isclose(TypeParam(NAN), TypeParam(NAN), 1.e9));
  EXPECT_FALSE(isclose(TypeParam(NAN), TypeParam(1.0), 1.e9));
  EXPECT_FALSE(isclose(TypeParam(1.0), TypeParam(NAN), 1.e9));
  EXPECT_FALSE(isclose(TypeParam(INFINITY), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose(TypeParam(1.0), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose(TypeParam(INFINITY), TypeParam(1.0), 1.e9));
  EXPECT_FALSE(isclose(-TypeParam(INFINITY), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose(-TypeParam(1.0), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose(-TypeParam(INFINITY), -TypeParam(1.0), 1.e9));
}

TYPED_TEST(IsCloseTest, value_equal_nans) {
  EXPECT_TRUE(isclose_equal_nan(TypeParam(NAN), TypeParam(NAN), 1.e9));
  EXPECT_FALSE(isclose_equal_nan(TypeParam(NAN), TypeParam(1.0), 1.e9));
  EXPECT_FALSE(isclose_equal_nan(TypeParam(1.0), TypeParam(NAN), 1.e9));
}
TYPED_TEST(IsCloseTest, value_equal_pos_infs) {
  EXPECT_TRUE(
      isclose_equal_nan(TypeParam(INFINITY), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose_equal_nan(TypeParam(1.0), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose_equal_nan(TypeParam(INFINITY), TypeParam(1.0), 1.e9));
}
TYPED_TEST(IsCloseTest, value_equal_neg_infs) {
  EXPECT_TRUE(
      isclose_equal_nan(-TypeParam(INFINITY), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose_equal_nan(-TypeParam(1.0), -TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(isclose_equal_nan(-TypeParam(INFINITY), -TypeParam(1.0), 1.e9));
}

TYPED_TEST(IsCloseTest, value_equal_infs_signbit) {
  EXPECT_FALSE(
      isclose_equal_nan(-TypeParam(INFINITY), TypeParam(INFINITY), 1.e9));
  EXPECT_FALSE(
      isclose_equal_nan(TypeParam(INFINITY), -TypeParam(INFINITY), 1.e9));
}

template <class Op> void do_isclose_units_test(Op op) {
  EXPECT_EQ(sc_units::none, op(sc_units::m, sc_units::m, sc_units::m));
  EXPECT_THROW_DISCARD(op(sc_units::m, sc_units::m, sc_units::s),
                       except::UnitError);
  EXPECT_THROW_DISCARD(op(sc_units::m, sc_units::s, sc_units::m),
                       except::UnitError);
  EXPECT_THROW_DISCARD(op(sc_units::s, sc_units::m, sc_units::m),
                       except::UnitError);
}

TEST(IsCloseTest, units) {
  do_isclose_units_test(isclose);
  do_isclose_units_test(isclose_equal_nan);
}

constexpr auto check_inplace = [](auto op, auto a, auto b, auto expected) {
  op(a, b);
  if (numeric::isnan(expected))
    EXPECT_TRUE(numeric::isnan(a));
  else
    EXPECT_EQ(a, expected);
};

TEST(ComparisonTest, min_max_support_time_point) {
  static_cast<void>(std::get<core::time_point>(decltype(max_equals)::types{}));
  static_cast<void>(std::get<core::time_point>(decltype(min_equals)::types{}));
  static_cast<void>(
      std::get<core::time_point>(decltype(nanmax_equals)::types{}));
  static_cast<void>(
      std::get<core::time_point>(decltype(nanmin_equals)::types{}));
}

TEST(ComparisonTest, max_equals) {
  check_inplace(max_equals, 1, 2, 2);
  check_inplace(max_equals, 2, 1, 2);
  check_inplace(max_equals, 1.2, 1.3, 1.3);
  check_inplace(max_equals, 1.3, 1.2, 1.3);
  check_inplace(max_equals, core::time_point(23), core::time_point(13),
                core::time_point(23));
  const double nan = NAN;
  check_inplace(max_equals, 3.1, nan, nan);
  check_inplace(max_equals, nan, 2.2, nan);
  check_inplace(max_equals, nan, nan, nan);
}

TEST(ComparisonTest, min_equals) {
  check_inplace(min_equals, 1, 2, 1);
  check_inplace(min_equals, 2, 1, 1);
  check_inplace(min_equals, 1.2, 1.3, 1.2);
  check_inplace(min_equals, 1.3, 1.2, 1.2);
  check_inplace(min_equals, core::time_point(23), core::time_point(13),
                core::time_point(13));
  const double nan = NAN;
  check_inplace(min_equals, 3.1, nan, nan);
  check_inplace(min_equals, nan, 2.2, nan);
  check_inplace(min_equals, nan, nan, nan);
}

TEST(ElementSpatialEqualTest, quaternion) {
  core::Quaternion x;
  EXPECT_TRUE(equal(x, x));
  EXPECT_FALSE(not_equal(x, x));
}

TEST(ElementSpatialEqualTest, translation) {
  core::Translation x;
  EXPECT_TRUE(equal(x, x));
  EXPECT_FALSE(not_equal(x, x));
}

TEST(ElementSpatialEqualTest, affine) {
  Eigen::Affine3d x(Eigen::Translation<double, 3>(Eigen::Vector3d(1, 2, 3)));
  EXPECT_TRUE(equal(x, x));
  EXPECT_FALSE(not_equal(x, x));
}
