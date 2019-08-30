// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(VariableTrigonometryTest, sin) {
  const auto rad = makeVariable<double>({}, units::rad, {pi<double>});
  const auto deg = makeVariable<double>({}, units::deg, {180.0});
  const auto expected =
      makeVariable<double>({}, units::dimensionless, {sin(pi<double>)});
  EXPECT_EQ(sin(rad), expected);
  EXPECT_EQ(sin(deg), expected);
}

TEST(VariableTrigonometryTest, cos) {
  const auto rad = makeVariable<double>({}, units::rad, {pi<double>});
  const auto deg = makeVariable<double>({}, units::deg, {180.0});
  const auto expected =
      makeVariable<double>({}, units::dimensionless, {cos(pi<double>)});
  EXPECT_EQ(cos(rad), expected);
  EXPECT_EQ(cos(deg), expected);
}

TEST(VariableTrigonometryTest, tan) {
  const auto rad = makeVariable<double>({}, units::rad, {pi<double>});
  const auto deg = makeVariable<double>({}, units::deg, {180.0});
  const auto expected =
      makeVariable<double>({}, units::dimensionless, {tan(pi<double>)});
  EXPECT_EQ(tan(rad), expected);
  EXPECT_EQ(tan(deg), expected);
}

TEST(VariableTrigonometryTest, asin) {
  const auto var = makeVariable<double>({}, units::dimensionless, {1.0});
  const auto expected =
      makeVariable<double>({}, units::rad, {0.5 * pi<double>});
  EXPECT_EQ(asin(var), expected);
}

TEST(VariableTrigonometryTest, acos) {
  const auto var = makeVariable<double>({}, units::dimensionless, {1.0});
  const auto expected = makeVariable<double>({}, units::rad, {0.0});
  EXPECT_EQ(acos(var), expected);
}

TEST(VariableTrigonometryTest, atan) {
  const auto var = makeVariable<double>({}, units::dimensionless, {1.0});
  const auto expected =
      makeVariable<double>({}, units::rad, {0.25 * pi<double>});
  EXPECT_EQ(atan(var), expected);
}

TEST(VariableTrigonometryTest, unit_fail) {
  EXPECT_THROW(sin(makeVariable<double>({}, units::dimensionless)),
               except::UnitError);
  EXPECT_THROW(cos(makeVariable<double>({}, units::dimensionless)),
               except::UnitError);
  EXPECT_THROW(tan(makeVariable<double>({}, units::dimensionless)),
               except::UnitError);
  EXPECT_THROW(asin(makeVariable<double>({}, units::rad)), except::UnitError);
  EXPECT_THROW(acos(makeVariable<double>({}, units::rad)), except::UnitError);
  EXPECT_THROW(atan(makeVariable<double>({}, units::rad)), except::UnitError);
}
