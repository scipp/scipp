// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/element/trigonometry.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementSinOutArgTest, unit_rad) {
  EXPECT_EQ(element::sin(sc_units::rad), sc_units::sin(sc_units::rad));
  EXPECT_EQ(element::sin(sc_units::deg), sc_units::sin(sc_units::deg));
  EXPECT_THROW_DISCARD(element::sin(sc_units::one), except::UnitError);
}

TEST(ElementSinOutArgTest, value_double) {
  EXPECT_EQ(element::sin(pi<double>), std::sin(pi<double>));
}

TEST(ElementSinOutArgTest, value_float) {
  EXPECT_EQ(element::sin(pi<float>), std::sin(pi<float>));
}

TEST(ElementSinOutArgTest, supported_types) {
  auto supported = decltype(element::sin)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementCosOutArgTest, unit_rad) {
  EXPECT_EQ(element::cos(sc_units::rad), sc_units::cos(sc_units::rad));
  EXPECT_EQ(element::cos(sc_units::deg), sc_units::cos(sc_units::deg));
  EXPECT_THROW_DISCARD(element::cos(sc_units::one), except::UnitError);
}

TEST(ElementCosOutArgTest, value_double) {
  EXPECT_EQ(element::cos(pi<double>), std::cos(pi<double>));
}

TEST(ElementCosOutArgTest, value_float) {
  EXPECT_EQ(element::cos(pi<float>), std::cos(pi<float>));
}

TEST(ElementCosOutArgTest, supported_types) {
  auto supported = decltype(element::cos)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementTanOutArgTest, unit_rad) {
  EXPECT_EQ(element::tan(sc_units::rad), sc_units::tan(sc_units::rad));
  EXPECT_EQ(element::tan(sc_units::deg), sc_units::tan(sc_units::deg));
  EXPECT_THROW_DISCARD(element::tan(sc_units::one), except::UnitError);
}

TEST(ElementTanOutArgTest, value_double) {
  EXPECT_EQ(element::tan(pi<double>), std::tan(pi<double>));
}

TEST(ElementTanOutArgTest, value_float) {
  EXPECT_EQ(element::tan(pi<float>), std::tan(pi<float>));
}

TEST(ElementTanOutArgTest, supported_types) {
  auto supported = decltype(element::tan)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementAsinTest, unit) {
  const sc_units::Unit dimensionless(sc_units::dimensionless);
  EXPECT_EQ(element::asin(dimensionless), sc_units::asin(dimensionless));
  const sc_units::Unit rad(sc_units::rad);
  EXPECT_THROW(element::asin(rad), except::UnitError);
}

TEST(ElementAsinTest, value) {
  EXPECT_EQ(element::asin(1.0), std::asin(1.0));
  EXPECT_EQ(element::asin(1.0f), std::asin(1.0f));
  EXPECT_TRUE(std::isnan(element::asin(1.2)));
  EXPECT_TRUE(std::isnan(element::asin(1.2f)));
}

TEST(ElementAsinOutArgTest, unit) {
  const sc_units::Unit dimensionless(sc_units::dimensionless);
  EXPECT_EQ(element::asin(dimensionless), sc_units::asin(dimensionless));
}

TEST(ElementAsinOutArgTest, value_double) {
  EXPECT_EQ(element::asin(1.0), std::asin(1.0));
}

TEST(ElementAsinOutArgTest, value_float) {
  EXPECT_EQ(element::asin(1.0f), std::asin(1.0f));
}

TEST(ElementAsinOutArgTest, supported_types) {
  auto supported = decltype(element::asin)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementAcosTest, unit) {
  const sc_units::Unit dimensionless(sc_units::dimensionless);
  EXPECT_EQ(element::acos(dimensionless), sc_units::acos(dimensionless));
  const sc_units::Unit rad(sc_units::rad);
  EXPECT_THROW(element::acos(rad), except::UnitError);
}

TEST(ElementAcosTest, value) {
  EXPECT_EQ(element::acos(1.0), std::acos(1.0));
  EXPECT_EQ(element::acos(1.0f), std::acos(1.0f));
  EXPECT_TRUE(std::isnan(element::acos(1.2)));
  EXPECT_TRUE(std::isnan(element::acos(1.2f)));
}

TEST(ElementAcosOutArgTest, unit) {
  const sc_units::Unit dimensionless(sc_units::dimensionless);
  EXPECT_EQ(element::acos(dimensionless), sc_units::acos(dimensionless));
}

TEST(ElementAcosOutArgTest, value_double) {
  EXPECT_EQ(element::acos(1.0), std::acos(1.0));
}

TEST(ElementAcosOutArgTest, value_float) {
  EXPECT_EQ(element::acos(1.0f), std::acos(1.0f));
}

TEST(ElementAcosOutArgTest, supported_types) {
  auto supported = decltype(element::acos)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

TEST(ElementAtanTest, unit) {
  const sc_units::Unit dimensionless(sc_units::dimensionless);
  EXPECT_EQ(element::atan(dimensionless), sc_units::atan(dimensionless));
  const sc_units::Unit rad(sc_units::rad);
  EXPECT_THROW(element::atan(rad), except::UnitError);
}

TEST(ElementAtanTest, value) {
  EXPECT_EQ(element::atan(1.0), std::atan(1.0));
  EXPECT_EQ(element::atan(1.0f), std::atan(1.0f));
}

TEST(ElementAtanOutArgTest, unit) {
  const sc_units::Unit dimensionless(sc_units::dimensionless);
  EXPECT_EQ(element::atan(dimensionless), sc_units::atan(dimensionless));
}

TEST(ElementAtanOutArgTest, value_double) {
  EXPECT_EQ(element::atan(1.0), std::atan(1.0));
}

TEST(ElementAtanOutArgTest, value_float) {
  EXPECT_EQ(element::atan(1.0f), std::atan(1.0f));
}

TEST(ElementAtanOutArgTest, supported_types) {
  auto supported = decltype(element::atan)::types{};
  static_cast<void>(std::get<double>(supported));
  static_cast<void>(std::get<float>(supported));
}

template <typename T> class ElementAtan2Test : public ::testing::Test {};
using ElementAtan2TestTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(ElementAtan2Test, ElementAtan2TestTypes);

TYPED_TEST(ElementAtan2Test, unit) {
  const sc_units::Unit m(sc_units::m);
  EXPECT_EQ(element::atan2(m, m), sc_units::atan2(m, m));
  const sc_units::Unit rad(sc_units::rad);
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
  const sc_units::Unit m(sc_units::m);
  const sc_units::Unit s(sc_units::s);
  EXPECT_EQ(element::atan2(m, m), sc_units::atan2(m, m));
  EXPECT_THROW(element::atan2(m, s), except::UnitError);
  EXPECT_THROW(element::atan2(s, m), except::UnitError);
}

TYPED_TEST(ElementAtan2Test, value_out) {
  using T = TypeParam;
  T y = 1;
  T x = 2;
  EXPECT_EQ(element::atan2(y, x), std::atan2(y, x));
}

TYPED_TEST(ElementAtan2Test, value_only_arguments_out) {
  using Op = decltype(element::atan2);
  EXPECT_TRUE((is_no_variance_arg<0, Op>())) << " y has variance ";
  EXPECT_TRUE((is_no_variance_arg<1, Op>())) << " x has variance ";
}
