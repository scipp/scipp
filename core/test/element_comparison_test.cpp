// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/comparison.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

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
