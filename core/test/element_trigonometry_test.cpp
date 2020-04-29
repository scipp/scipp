// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/element/trigonometry.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

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

template <typename T> class ElementAtan2Test : public ::testing::Test {};
using ElementAtan2TestTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(ElementAtan2Test, ElementAtan2TestTypes);

TYPED_TEST(ElementAtan2Test, unit) {
  const units::Unit m(units::m);
  EXPECT_EQ(element::atan2(m, m), units::atan2(m, m));
  const units::Unit rad(units::rad);
  EXPECT_THROW(element::atan2(rad, m), except::UnitError);
}

TYPED_TEST(ElementAtan2Test, value) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(element::atan2(y, x), std::atan2(y, x));
  x = -1;
  EXPECT_EQ(element::atan2(y, x), std::atan2(y, x));
}

template <int T, typename Op> bool is_no_variance_arg() {
  return std::is_base_of_v<transform_flags::expect_no_variance_arg_t<T>, Op>;
}

TYPED_TEST(ElementAtan2Test, value_only_arguments) {
  using Op = decltype(element::atan2);
  EXPECT_TRUE((is_no_variance_arg<0, Op>())) << " y has variance ";
  EXPECT_TRUE((is_no_variance_arg<1, Op>())) << " x has variance ";
}

TYPED_TEST(ElementAtan2Test, unit_out) {
  const units::Unit m(units::m);
  const units::Unit s(units::s);
  units::Unit out(units::dimensionless);
  element::atan2_out_arg(out, m, m);
  EXPECT_EQ(out, units::atan2(m, m));
  EXPECT_THROW(element::atan2_out_arg(out, m, s), except::UnitError);
  EXPECT_THROW(element::atan2_out_arg(out, s, m), except::UnitError);
}

TYPED_TEST(ElementAtan2Test, value_out) {
  using T = TypeParam;
  T out = -1;
  T y = 1;
  T x = 2;
  element::atan2_out_arg(out, y, x);
  EXPECT_EQ(out, std::atan2(y, x));
}

TYPED_TEST(ElementAtan2Test, value_only_arguments_out) {
  using Op = decltype(element::atan2_out_arg);
  EXPECT_TRUE((is_no_variance_arg<0, Op>())) << " out has variance ";
  EXPECT_TRUE((is_no_variance_arg<1, Op>())) << " y has variance ";
  EXPECT_TRUE((is_no_variance_arg<2, Op>())) << " x has variance ";
}
