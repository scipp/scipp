// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "../element_unary_operations.h"

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

TEST(ElementSinOutArgTest, unit_rad) {
  const units::Unit rad(units::rad);
  units::Unit out(units::dimensionless);
  element::sin_out_arg(out, rad);
  EXPECT_EQ(out, units::sin(rad));
}

TEST(ElementSinOutArgTest, unit_deg) {
  const units::Unit deg(units::deg);
  units::Unit out(units::dimensionless);
  element::sin_out_arg(out, deg);
  EXPECT_EQ(out, units::sin(deg));
}

TEST(ElementSinOutArgTest, value_double) {
  double out;
  element::sin_out_arg(out, pi<double>);
  EXPECT_EQ(out, std::sin(pi<double>));
}

TEST(ElementSinOutArgTest, value_float) {
  float out;
  element::sin_out_arg(out, pi<float>);
  EXPECT_EQ(out, std::sin(pi<float>));
}

TEST(ElementSinOutArgTest, supported_types) {
  auto supported = decltype(element::sin_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementCosOutArgTest, unit_rad) {
  const units::Unit rad(units::rad);
  units::Unit out(units::dimensionless);
  element::cos_out_arg(out, rad);
  EXPECT_EQ(out, units::cos(rad));
}

TEST(ElementCosOutArgTest, unit_deg) {
  const units::Unit deg(units::deg);
  units::Unit out(units::dimensionless);
  element::cos_out_arg(out, deg);
  EXPECT_EQ(out, units::cos(deg));
}

TEST(ElementCosOutArgTest, value_double) {
  double out;
  element::cos_out_arg(out, pi<double>);
  EXPECT_EQ(out, std::cos(pi<double>));
}

TEST(ElementCosOutArgTest, value_float) {
  float out;
  element::cos_out_arg(out, pi<float>);
  EXPECT_EQ(out, std::cos(pi<float>));
}

TEST(ElementCosOutArgTest, supported_types) {
  auto supported = decltype(element::cos_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementTanOutArgTest, unit_rad) {
  const units::Unit rad(units::rad);
  units::Unit out(units::dimensionless);
  element::tan_out_arg(out, rad);
  EXPECT_EQ(out, units::tan(rad));
}

TEST(ElementTanOutArgTest, unit_deg) {
  const units::Unit deg(units::deg);
  units::Unit out(units::dimensionless);
  element::tan_out_arg(out, deg);
  EXPECT_EQ(out, units::tan(deg));
}

TEST(ElementTanOutArgTest, value_double) {
  double out;
  element::tan_out_arg(out, pi<double>);
  EXPECT_EQ(out, std::tan(pi<double>));
}

TEST(ElementTanOutArgTest, value_float) {
  float out;
  element::tan_out_arg(out, pi<float>);
  EXPECT_EQ(out, std::tan(pi<float>));
}

TEST(ElementTanOutArgTest, supported_types) {
  auto supported = decltype(element::tan_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementAsinTest, unit) {
  const units::Unit dimensionless(units::dimensionless);
  EXPECT_EQ(element::asin(dimensionless), units::asin(dimensionless));
  const units::Unit rad(units::rad);
  EXPECT_THROW(element::asin(rad), except::UnitError);
}

TEST(ElementAsinTest, value) {
  EXPECT_EQ(element::asin(1.0), std::asin(1.0));
  EXPECT_EQ(element::asin(1.0f), std::asin(1.0f));
}

TEST(ElementAsinOutArgTest, unit) {
  const units::Unit dimensionless(units::dimensionless);
  units::Unit out(units::dimensionless);
  element::asin_out_arg(out, dimensionless);
  EXPECT_EQ(out, units::asin(dimensionless));
}

TEST(ElementAsinOutArgTest, value_double) {
  double out;
  element::asin_out_arg(out, 1.0);
  EXPECT_EQ(out, std::asin(1.0));
}

TEST(ElementAsinOutArgTest, value_float) {
  float out;
  element::asin_out_arg(out, 1.0f);
  EXPECT_EQ(out, std::asin(1.0f));
}

TEST(ElementAsinOutArgTest, supported_types) {
  auto supported = decltype(element::asin_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementAcosTest, unit) {
  const units::Unit dimensionless(units::dimensionless);
  EXPECT_EQ(element::acos(dimensionless), units::acos(dimensionless));
  const units::Unit rad(units::rad);
  EXPECT_THROW(element::acos(rad), except::UnitError);
}

TEST(ElementAcosTest, value) {
  EXPECT_EQ(element::acos(1.0), std::acos(1.0));
  EXPECT_EQ(element::acos(1.0f), std::acos(1.0f));
}

TEST(ElementAcosOutArgTest, unit) {
  const units::Unit dimensionless(units::dimensionless);
  units::Unit out(units::dimensionless);
  element::acos_out_arg(out, dimensionless);
  EXPECT_EQ(out, units::acos(dimensionless));
}

TEST(ElementAcosOutArgTest, value_double) {
  double out;
  element::acos_out_arg(out, 1.0);
  EXPECT_EQ(out, std::acos(1.0));
}

TEST(ElementAcosOutArgTest, value_float) {
  float out;
  element::acos_out_arg(out, 1.0f);
  EXPECT_EQ(out, std::acos(1.0f));
}

TEST(ElementAcosOutArgTest, supported_types) {
  auto supported = decltype(element::acos_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}

TEST(ElementAtanTest, unit) {
  const units::Unit dimensionless(units::dimensionless);
  EXPECT_EQ(element::atan(dimensionless), units::atan(dimensionless));
  const units::Unit rad(units::rad);
  EXPECT_THROW(element::atan(rad), except::UnitError);
}

TEST(ElementAtanTest, value) {
  EXPECT_EQ(element::atan(1.0), std::atan(1.0));
  EXPECT_EQ(element::atan(1.0f), std::atan(1.0f));
}

TEST(ElementAtanOutArgTest, unit) {
  const units::Unit dimensionless(units::dimensionless);
  units::Unit out(units::dimensionless);
  element::atan_out_arg(out, dimensionless);
  EXPECT_EQ(out, units::atan(dimensionless));
}

TEST(ElementAtanOutArgTest, value_double) {
  double out;
  element::atan_out_arg(out, 1.0);
  EXPECT_EQ(out, std::atan(1.0));
}

TEST(ElementAtanOutArgTest, value_float) {
  float out;
  element::atan_out_arg(out, 1.0f);
  EXPECT_EQ(out, std::atan(1.0f));
}

TEST(ElementAtanOutArgTest, supported_types) {
  auto supported = decltype(element::atan_out_arg)::types{};
  std::get<double>(supported);
  std::get<float>(supported);
}
