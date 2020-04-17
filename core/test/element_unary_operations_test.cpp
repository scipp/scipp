// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/unary_operations.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementAbsTest, unit) {
  units::Unit m(units::m);
  EXPECT_EQ(element::abs(m), units::abs(m));
}

TEST(ElementAbsTest, value) {
  EXPECT_EQ(element::abs(-1.23), std::abs(-1.23));
  EXPECT_EQ(element::abs(-1.23456789f), std::abs(-1.23456789f));
}

TEST(ElementAbsTest, value_and_variance) {
  const ValueAndVariance x(-2.0, 1.0);
  EXPECT_EQ(element::abs(x), abs(x));
}

TEST(ElementAbsOutArgTest, unit) {
  units::Unit m(units::m);
  units::Unit out(units::dimensionless);
  element::abs_out_arg(out, m);
  EXPECT_EQ(out, units::abs(m));
}

TEST(ElementAbsOutArgTest, value_double) {
  double out;
  element::abs_out_arg(out, -1.23);
  EXPECT_EQ(out, std::abs(-1.23));
}

TEST(ElementAbsOutArgTest, value_float) {
  float out;
  element::abs_out_arg(out, -1.23456789f);
  EXPECT_EQ(out, std::abs(-1.23456789f));
}

TEST(ElementAbsOutArgTest, value_and_variance) {
  const ValueAndVariance x(-2.0, 1.0);
  ValueAndVariance out(x);
  element::abs_out_arg(out, x);
  EXPECT_EQ(out, abs(x));
}

TEST(ElementAbsOutArgTest, supported_types) {
  auto supported = decltype(element::abs_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementSqrtTest, unit) {
  const units::Unit m2(units::m * units::m);
  EXPECT_EQ(element::sqrt(m2), units::sqrt(m2));
}

TEST(ElementSqrtTest, value) {
  EXPECT_EQ(element::sqrt(1.23), std::sqrt(1.23));
  EXPECT_EQ(element::sqrt(1.23456789f), std::sqrt(1.23456789f));
}

TEST(ElementSqrtTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::sqrt(x), sqrt(x));
}

TEST(ElementSqrtOutArgTest, unit) {
  const units::Unit m2(units::m * units::m);
  units::Unit out(units::dimensionless);
  element::sqrt_out_arg(out, m2);
  EXPECT_EQ(out, units::sqrt(m2));
}

TEST(ElementSqrtOutArgTest, value_double) {
  double out;
  element::sqrt_out_arg(out, 1.23);
  EXPECT_EQ(out, std::sqrt(1.23));
}

TEST(ElementSqrtOutArgTest, value_float) {
  float out;
  element::sqrt_out_arg(out, 1.23456789f);
  EXPECT_EQ(out, std::sqrt(1.23456789f));
}

TEST(ElementSqrtOutArgTest, value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  ValueAndVariance out(x);
  element::sqrt_out_arg(out, x);
  EXPECT_EQ(out, sqrt(x));
}

TEST(ElementSqrtOutArgTest, supported_types) {
  auto supported = decltype(element::sqrt_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

template <typename T> class ElementNanToNumTest : public ::testing::Test {};
using ElementReplacementTestTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(ElementNanToNumTest, ElementReplacementTestTypes);

template <typename T, typename Op>
void targetted_replacement_test(Op op, const T &replaceable,
                                const T &nonreplaceable, const T &replacement) {
  EXPECT_EQ(replacement, op(replaceable, replacement));
  EXPECT_EQ(nonreplaceable, op(nonreplaceable,
                               replacement)); // No replacement expected
}
template <typename T, typename Op>
void targetted_replacement_out_arg_test(Op op, T &out, const T &replaceable,
                                        const T &nonreplaceable,
                                        const T &replacement) {
  op(out, replaceable, replacement);
  EXPECT_EQ(replacement, out);
  op(out, nonreplaceable, replacement);
  EXPECT_EQ(nonreplaceable, out);
}

template <typename Op> void targetted_unit_test(Op op) {
  units::Unit m(units::m);
  EXPECT_EQ(m, op(m, m));
  units::Unit s(units::s);
  EXPECT_THROW(op(s, m), except::UnitError);
}

template <typename Op> void targetted_unit_test_out(Op op) {
  units::Unit m(units::m);
  units::Unit u;
  op(u, m, m);
  EXPECT_EQ(m, u);
  units::Unit s(units::s);
  EXPECT_THROW(op(u, s, m), except::UnitError);
}

TYPED_TEST(ElementNanToNumTest, unit) {
  targetted_unit_test(element::nan_to_num);
}

TYPED_TEST(ElementNanToNumTest, value) {
  using T = TypeParam;
  const T replaceable = NAN;
  const T replacement = 1.0;
  const T nonreplaceable = 2.0;
  targetted_replacement_test(element::nan_to_num, replaceable, nonreplaceable,
                             replacement);
}

TYPED_TEST(ElementNanToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> replaceable(NAN, 0.1);
  const ValueAndVariance<T> replacement(1, 1);
  const ValueAndVariance<T> nonreplaceable(2, 2);
  targetted_replacement_test(element::nan_to_num, replaceable, nonreplaceable,
                             replacement);
}

TYPED_TEST(ElementNanToNumTest, unit_out) {
  targetted_unit_test_out(element::nan_to_num_out_arg);
}

TYPED_TEST(ElementNanToNumTest, value_out) {
  using T = TypeParam;
  const T replaceable = NAN;
  const T replacement = 1;
  const T nonreplaceable = 2;
  T out = -1;
  targetted_replacement_out_arg_test(element::nan_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
TYPED_TEST(ElementNanToNumTest, value_and_variance_out) {
  using T = TypeParam;
  ValueAndVariance<T> replaceable(NAN, 2);
  const ValueAndVariance<T> nonreplaceable(3, 3);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  targetted_replacement_out_arg_test(element::nan_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}

template <typename T>
class ElementPositiveInfToNumTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementPositiveInfToNumTest, ElementReplacementTestTypes);

TYPED_TEST(ElementPositiveInfToNumTest, unit) {
  targetted_unit_test(element::positive_inf_to_num);
}

TYPED_TEST(ElementPositiveInfToNumTest, value) {
  using T = TypeParam;
  const T replacement = 1.0;
  const T replaceable = INFINITY;
  const T nonreplaceable = -INFINITY;
  targetted_replacement_test(element::positive_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}

TYPED_TEST(ElementPositiveInfToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> replaceable(INFINITY, 1);
  const ValueAndVariance<T> replacement(1, 1);
  const ValueAndVariance<T> nonreplaceable(-INFINITY, 1);
  targetted_replacement_test(element::positive_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}
TYPED_TEST(ElementPositiveInfToNumTest, unit_out) {
  targetted_unit_test_out(element::positive_inf_to_num_out_arg);
}
TYPED_TEST(ElementPositiveInfToNumTest, value_out) {
  using T = TypeParam;
  T out = -1;
  const T replaceable = INFINITY;
  const T replacement = 1;
  const T nonreplaceable = -INFINITY;
  targetted_replacement_out_arg_test(element::positive_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
TYPED_TEST(ElementPositiveInfToNumTest, value_and_variance_out) {
  using T = TypeParam;
  ValueAndVariance<T> replaceable(INFINITY, 2);
  const ValueAndVariance<T> nonreplaceable(-INFINITY, 3);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  targetted_replacement_out_arg_test(element::positive_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}

template <typename T>
class ElementNegativeInfToNumTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementNegativeInfToNumTest, ElementReplacementTestTypes);

TYPED_TEST(ElementNegativeInfToNumTest, unit) {
  targetted_unit_test(element::negative_inf_to_num);
}
TYPED_TEST(ElementNegativeInfToNumTest, value) {
  using T = TypeParam;
  const T replacement = 1.0;
  const T replaceable = -INFINITY;
  const T nonreplaceable = INFINITY;
  targetted_replacement_test(element::negative_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}

TYPED_TEST(ElementNegativeInfToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> replaceable(-INFINITY, 1);
  const ValueAndVariance<T> replacement(1, 1);
  const ValueAndVariance<T> nonreplaceable(INFINITY, 1);
  targetted_replacement_test(element::negative_inf_to_num, replaceable,
                             nonreplaceable, replacement);
}

TYPED_TEST(ElementNegativeInfToNumTest, unit_out) {
  targetted_unit_test_out(element::negative_inf_to_num_out_arg);
}
TYPED_TEST(ElementNegativeInfToNumTest, value_out) {
  using T = TypeParam;
  T out = -1;
  const T replaceable = -INFINITY;
  const T replacement = 1;
  const T nonreplaceable = INFINITY;
  targetted_replacement_out_arg_test(element::negative_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
TYPED_TEST(ElementNegativeInfToNumTest, value_and_variance_out) {
  using T = TypeParam;
  ValueAndVariance<T> replaceable(-INFINITY, 2);
  const ValueAndVariance<T> nonreplaceable(INFINITY, 3);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  targetted_replacement_out_arg_test(element::negative_inf_to_num_out_arg, out,
                                     replaceable, nonreplaceable, replacement);
}
