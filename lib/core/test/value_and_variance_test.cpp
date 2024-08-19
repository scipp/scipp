// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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

TEST(ValueAndVarianceTest, unary_exp) {
  const ValueAndVariance a{2.0, 1.0};
  const auto b = exp(a);
  EXPECT_EQ(b.value, std::exp(a.value));
  EXPECT_EQ(b.variance, b.value * b.value * a.variance);
}

TEST(ValueAndVarianceTest, unary_log) {
  const ValueAndVariance a{2.0, 1.0};
  const auto b = log(a);
  EXPECT_EQ(b.value, std::log(a.value));
  EXPECT_EQ(b.variance, a.variance / a.value / a.value);
}

TEST(ValueAndVarianceTest, unary_log10) {
  const ValueAndVariance a{2.0, 1.0};
  const auto b = log10(a);
  EXPECT_EQ(b.value, std::log10(a.value));
  EXPECT_EQ(b.variance,
            a.variance / a.value / a.value / std::log(10.0) / std::log(10.0));
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

TEST(ValueAndVarianceTest, binary_pow) {
  const ValueAndVariance base{3.0, 2.0};
  auto result = pow(base, 3);
  EXPECT_NEAR(27.0, result.value, 1e-15);
  // pow.var = (3 * (base.val ^ 2)) ^ 2 * base.var
  EXPECT_NEAR(std::pow(3 * 9.0, 2.0) * base.variance, result.variance, 1e-13);

  result = pow(base, 1);
  EXPECT_NEAR(base.value, result.value, 1e-15);
  EXPECT_NEAR(base.variance, result.variance, 1e-15);

  result = pow(base, 0);
  EXPECT_NEAR(1.0, result.value, 1e-15);
  EXPECT_NEAR(0.0, result.variance, 1e-15);

  result = pow(base, -2);
  EXPECT_NEAR(1.0 / 9.0, result.value, 1e-16);
  // pow.var = (|-2| * (base.val ^ -3)) ^ 2 * base.var
  EXPECT_NEAR(std::pow(2 / 27.0, 2.0) * base.variance, result.variance, 1e-16);

  const ValueAndVariance zero{0.0, 1.0};
  result = pow(zero, 0.5);
  EXPECT_NEAR(0., result.value, 1e-15);
  EXPECT_TRUE(std::isinf(result.variance));

  result = pow(zero, 0.);
  EXPECT_NEAR(1., result.value, 1e-15);

  const ValueAndVariance zerozero{0.0, 0.0};
  result = pow(zerozero, 0.5);
  EXPECT_NEAR(0., result.value, 1e-15);
  EXPECT_TRUE(std::isnan(result.variance));
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

TYPED_TEST(ValueAndVarianceBinaryOpTest, int_scalar_lhs_valueandvariance_rhs) {
  const ValueAndVariance lhs{5.0, 0.0};
  const ValueAndVariance rhs{8.0, 2.0};

  const auto expected = TestFixture::op(lhs, rhs);
  const auto result = TestFixture::op(static_cast<int32_t>(lhs.value), rhs);

  EXPECT_EQ(expected.value, result.value);
  EXPECT_EQ(expected.variance, result.variance);
}

TYPED_TEST(ValueAndVarianceBinaryOpTest, valueandvariance_lhs_int_scalar_rhs) {
  const ValueAndVariance lhs{5.0, 1.0};
  const ValueAndVariance rhs{8.0, 0.0};

  const auto expected = TestFixture::op(lhs, rhs);
  const auto result = TestFixture::op(lhs, static_cast<int32_t>(rhs.value));

  EXPECT_EQ(expected.value, result.value);
  EXPECT_EQ(expected.variance, result.variance);
}

TYPED_TEST(ValueAndVarianceBinaryOpTest, no_int_overflow_lhs) {
  const int32_t lhs_value = 1615722;
  const ValueAndVariance lhs{static_cast<double>(lhs_value), 0.0};
  const ValueAndVariance rhs{419.0, 419.0};

  const auto expected = TestFixture::op(lhs, rhs);
  const auto result = TestFixture::op(lhs_value, rhs);

  EXPECT_EQ(expected.value, result.value);
  EXPECT_EQ(expected.variance, result.variance);
}

TYPED_TEST(ValueAndVarianceBinaryOpTest, no_int_overflow_rhs) {
  const int32_t rhs_value = 1615722;
  const ValueAndVariance lhs{419.0, 419.0};
  const ValueAndVariance rhs{static_cast<double>(rhs_value), 0.0};

  const auto expected = TestFixture::op(lhs, rhs);
  const auto result = TestFixture::op(lhs, rhs_value);

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
