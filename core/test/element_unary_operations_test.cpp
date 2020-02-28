// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "../element_unary_operations.h"
#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

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

TYPED_TEST(ElementNanToNumTest, value) {
  using T = TypeParam;
  const T replacement = 1.0;
  const T original = 2.0;
  EXPECT_EQ(replacement, element::nan_to_num(T(NAN), replacement));
  EXPECT_EQ(original, element::nan_to_num(
                          original, replacement)); // No replacement expected
}

TYPED_TEST(ElementNanToNumTest, value_and_variance) {
  using T = TypeParam;
  const ValueAndVariance<T> input(NAN, 0.1);
  const ValueAndVariance<T> replacement(1, 1);
  EXPECT_EQ(replacement, element::nan_to_num(input, replacement));
  const ValueAndVariance<T> original(2, 2);
  EXPECT_EQ(original, element::nan_to_num(
                          original, replacement)); // No replacement expected
}

template <typename T> class ElementNanToNumOutTest : public ::testing::Test {};
TYPED_TEST_SUITE(ElementNanToNumOutTest, ElementReplacementTestTypes);

TYPED_TEST(ElementNanToNumOutTest, value) {
  using T = TypeParam;
  T out = -1;
  const T replacement = 1;
  element::nan_to_num_out_arg(out, double(NAN), replacement);
  EXPECT_EQ(replacement, out);
}
TYPED_TEST(ElementNanToNumOutTest, value_and_variance) {
  using T = TypeParam;
  ValueAndVariance<T> input(NAN, 2);
  ValueAndVariance<T> out(-1, -1);
  const ValueAndVariance<T> replacement(1, 1);
  element::nan_to_num_out_arg(out, input, replacement);
  EXPECT_EQ(replacement, out);
  const ValueAndVariance<T> original(3, 3);
  element::nan_to_num_out_arg(out, original, replacement);
  EXPECT_EQ(original, out);
}
