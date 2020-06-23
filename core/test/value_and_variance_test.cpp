// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_operations.h"

#include "fix_typed_test_suite_warnings.h"
#include "scipp/core/value_and_variance.h"

using namespace scipp;
using namespace scipp::core;

TEST(ValueAndVarianceTest, unary_negate) {
  const ValueAndVariance a{5.0, 1.0};
  const auto b = -a;
  EXPECT_EQ(-5.0, b.value);
  EXPECT_EQ(1.0, b.variance);
}

TEST(ValueAndVarianceTest, unary_sqrt) {
  const ValueAndVariance a{25.0, 5.0};
  const auto b = sqrt(a);
  EXPECT_EQ(5.0, b.value);
  EXPECT_EQ(0.25 * (5.0 / 25.0), b.variance);
}

TEST(ValueAndVarianceTest, unary_abs) {
  const ValueAndVariance a{-5.0, 1.0};
  const auto b = abs(a);
  EXPECT_EQ(5.0, b.value);
  EXPECT_EQ(1.0, b.variance);
}

TEST(ValueAndVarianceTest, binary_plus) {
  const ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  const auto result = lhs + rhs;
  EXPECT_EQ(lhs.value + rhs.value, result.value);
  EXPECT_EQ(3.0, result.variance);
}

TEST(ValueAndVarianceTest, binary_plus_equals) {
  ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  lhs += rhs;
  EXPECT_EQ(5.0 + 8.0, lhs.value);
  EXPECT_EQ(3.0, lhs.variance);
}

TEST(ValueAndVarianceTest, binary_minus) {
  const ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  const auto result = lhs - rhs;
  EXPECT_EQ(lhs.value - rhs.value, result.value);
  EXPECT_EQ(3.0, result.variance);
}

TEST(ValueAndVarianceTest, binary_minus_equals) {
  ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  lhs -= rhs;
  EXPECT_EQ(5.0 - 8.0, lhs.value);
  EXPECT_EQ(3.0, lhs.variance);
}

TEST(ValueAndVarianceTest, binary_times) {
  const ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  const auto result = lhs * rhs;
  EXPECT_EQ(lhs.value * rhs.value, result.value);
  EXPECT_EQ(1.0 * 8.0 * 8.0 + 2.0 * 5.0 * 5.0, result.variance);
}

TEST(ValueAndVarianceTest, binary_times_equals) {
  ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  lhs *= rhs;
  EXPECT_EQ(5.0 * 8.0, lhs.value);
  EXPECT_EQ(1.0 * 8.0 * 8.0 + 2.0 * 5.0 * 5.0, lhs.variance);
}

TEST(ValueAndVarianceTest, binary_divide) {
  const ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  const auto result = lhs / rhs;
  EXPECT_EQ(lhs.value / rhs.value, result.value);
  EXPECT_EQ((1.0 + 2.0 * (5.0 * 5.0) / (8.0 * 8.0)) / (8.0 * 8.0),
            result.variance);
}

TEST(ValueAndVarianceTest, binary_divide_equals) {
  ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 2.0};
  lhs /= rhs;
  EXPECT_EQ(5.0 / 8.0, lhs.value);
  EXPECT_EQ((1.0 + 2.0 * (5.0 * 5.0) / (8.0 * 8.0)) / (8.0 * 8.0),
            lhs.variance);
}

TEST(ValueAndVarianceTest, comparison) {
  ValueAndVariance a1{1.0, 2.0};
  ValueAndVariance a2{1.0, 3.0}; // same as a1 but difference variance
  ValueAndVariance b{2.0, 2.0};

  EXPECT_TRUE(a1 == a1);
  EXPECT_TRUE(a1 == a2);
  EXPECT_FALSE(a1 == b);

  EXPECT_FALSE(a1 != a1);
  EXPECT_FALSE(a1 != a2);
  EXPECT_TRUE(a1 != b);

  EXPECT_FALSE(a1 < a1);
  EXPECT_FALSE(a1 < a2);
  EXPECT_TRUE(a1 < b);
  EXPECT_FALSE(b < a1);

  EXPECT_FALSE(a1 > a1);
  EXPECT_FALSE(a1 > a2);
  EXPECT_FALSE(a1 > b);
  EXPECT_TRUE(b > a1);

  EXPECT_TRUE(a1 <= a1);
  EXPECT_TRUE(a1 <= a2);
  EXPECT_TRUE(a1 <= b);
  EXPECT_FALSE(b <= a1);

  EXPECT_TRUE(a1 >= a1);
  EXPECT_TRUE(a1 >= a2);
  EXPECT_FALSE(a1 >= b);
  EXPECT_TRUE(b >= a1);
}

TEST(ValueAndVarianceTest, comparison_no_variance_lhs) {
  ValueAndVariance a1{1.0, 2.0};
  ValueAndVariance a2{1.0, 3.0}; // same as a1 but difference variance
  ValueAndVariance b{2.0, 2.0};

  EXPECT_TRUE(a1.value == a1);
  EXPECT_TRUE(a1.value == a2);
  EXPECT_FALSE(a1.value == b);

  EXPECT_FALSE(a1.value != a1);
  EXPECT_FALSE(a1.value != a2);
  EXPECT_TRUE(a1.value != b);

  EXPECT_FALSE(a1.value < a1);
  EXPECT_FALSE(a1.value < a2);
  EXPECT_TRUE(a1.value < b);
  EXPECT_FALSE(b.value < a1);

  EXPECT_FALSE(a1.value > a1);
  EXPECT_FALSE(a1.value > a2);
  EXPECT_FALSE(a1.value > b);
  EXPECT_TRUE(b.value > a1);

  EXPECT_TRUE(a1.value <= a1);
  EXPECT_TRUE(a1.value <= a2);
  EXPECT_TRUE(a1.value <= b);
  EXPECT_FALSE(b.value <= a1);

  EXPECT_TRUE(a1.value >= a1);
  EXPECT_TRUE(a1.value >= a2);
  EXPECT_FALSE(a1.value >= b);
  EXPECT_TRUE(b.value >= a1);
}

TEST(ValueAndVarianceTest, comparison_no_variance_rhs) {
  ValueAndVariance a1{1.0, 2.0};
  ValueAndVariance a2{1.0, 3.0}; // same as a1 but difference variance
  ValueAndVariance b{2.0, 2.0};

  EXPECT_TRUE(a1 == a1.value);
  EXPECT_TRUE(a1 == a2.value);
  EXPECT_FALSE(a1 == b.value);

  EXPECT_FALSE(a1 != a1.value);
  EXPECT_FALSE(a1 != a2.value);
  EXPECT_TRUE(a1 != b.value);

  EXPECT_FALSE(a1 < a1.value);
  EXPECT_FALSE(a1 < a2.value);
  EXPECT_TRUE(a1 < b.value);
  EXPECT_FALSE(b < a1.value);

  EXPECT_FALSE(a1 > a1.value);
  EXPECT_FALSE(a1 > a2.value);
  EXPECT_FALSE(a1 > b.value);
  EXPECT_TRUE(b > a1.value);

  EXPECT_TRUE(a1 <= a1.value);
  EXPECT_TRUE(a1 <= a2.value);
  EXPECT_TRUE(a1 <= b.value);
  EXPECT_FALSE(b <= a1.value);

  EXPECT_TRUE(a1 >= a1.value);
  EXPECT_TRUE(a1 >= a2.value);
  EXPECT_FALSE(a1 >= b.value);
  EXPECT_TRUE(b >= a1.value);
}

/* This test suite tests for equality between ValueAndVariance-scalar binary
 * operations and the equivalent ValueAndVariance-ValueAndVariance operation. */
/* The assumption is made that ValueAndVariance-ValueAndVariance binary
 * operations are correct. */
template <class Op>
class ValueAndVarianceBinaryOpTest : public ::testing::Test,
                                     public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

TYPED_TEST_SUITE(ValueAndVarianceBinaryOpTest, Binary);

TYPED_TEST(ValueAndVarianceBinaryOpTest, scalar_lhs_valueandvariance_rhs) {
  const ValueAndVariance lhs{5.0, 0.0};
  const ValueAndVariance rhs{8.0, 2.0};

  const auto expected = TestFixture::op(lhs, rhs);
  const auto result = TestFixture::op(lhs.value, rhs);

  EXPECT_EQ(expected.value, result.value);
  EXPECT_EQ(expected.variance, result.variance);
}

TYPED_TEST(ValueAndVarianceBinaryOpTest, valueandvariance_lhs_scalar_rhs) {
  const ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 0.0};

  const auto expected = TestFixture::op(lhs, rhs);
  const auto result = TestFixture::op(lhs, rhs.value);

  EXPECT_EQ(expected.value, result.value);
  EXPECT_EQ(expected.variance, result.variance);
}

/* This test suite tests for equality between ValueAndVariance-scalar binary
 * equals operations and the equivalent ValueAndVariance-ValueAndVariance
 * operation. */
/* The assumption is made that ValueAndVariance-ValueAndVariance binary equals
 * operations are correct. */
template <class Op>
class ValueAndVarianceBinaryEqualsOpTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

TYPED_TEST_SUITE(ValueAndVarianceBinaryEqualsOpTest, BinaryEquals);

TYPED_TEST(ValueAndVarianceBinaryEqualsOpTest,
           valueandvariance_lhs_scalar_rhs) {
  ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 0.0};

  auto expected{lhs};
  TestFixture::op(expected, rhs);

  TestFixture::op(lhs, rhs.value);

  EXPECT_EQ(expected.value, lhs.value);
  EXPECT_EQ(expected.variance, lhs.variance);
}
