// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "../element_unary_operations.h"

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
