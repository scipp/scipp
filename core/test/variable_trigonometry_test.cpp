// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(VariableTrigonometryTest, sin) {
  const auto rad = createVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{pi<double>});
  const auto deg = createVariable<double>(
      Dims(), Shape(), units::Unit(units::deg), Values{180.0});
  const auto expected =
      createVariable<double>(Dims(), Shape(), units::Unit(units::dimensionless),
                             Values{sin(pi<double>)});
  EXPECT_EQ(sin(rad), expected);
  EXPECT_EQ(sin(deg), expected);
}

TEST(VariableTrigonometryTest, cos) {
  const auto rad = createVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{pi<double>});
  const auto deg = createVariable<double>(
      Dims(), Shape(), units::Unit(units::deg), Values{180.0});
  const auto expected =
      createVariable<double>(Dims(), Shape(), units::Unit(units::dimensionless),
                             Values{cos(pi<double>)});
  EXPECT_EQ(cos(rad), expected);
  EXPECT_EQ(cos(deg), expected);
}

TEST(VariableTrigonometryTest, tan) {
  const auto rad = createVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{pi<double>});
  const auto deg = createVariable<double>(
      Dims(), Shape(), units::Unit(units::deg), Values{180.0});
  const auto expected =
      createVariable<double>(Dims(), Shape(), units::Unit(units::dimensionless),
                             Values{tan(pi<double>)});
  EXPECT_EQ(tan(rad), expected);
  EXPECT_EQ(tan(deg), expected);
}

TEST(VariableTrigonometryTest, asin) {
  const auto var = createVariable<double>(
      Dims(), Shape(), units::Unit(units::dimensionless), Values{1.0});
  const auto expected = createVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{0.5 * pi<double>});
  EXPECT_EQ(asin(var), expected);
}

TEST(VariableTrigonometryTest, acos) {
  const auto var = createVariable<double>(
      Dims(), Shape(), units::Unit(units::dimensionless), Values{1.0});
  const auto expected = createVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{0.0});
  EXPECT_EQ(acos(var), expected);
}

TEST(VariableTrigonometryTest, atan) {
  const auto var = createVariable<double>(
      Dims(), Shape(), units::Unit(units::dimensionless), Values{1.0});
  const auto expected = createVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{0.25 * pi<double>});
  EXPECT_EQ(atan(var), expected);
}

TEST(VariableTrigonometryTest, unit_fail) {
  EXPECT_THROW(sin(createVariable<double>(Dims(), Shape(),
                                          units::Unit(units::dimensionless))),
               except::UnitError);
  EXPECT_THROW(cos(createVariable<double>(Dims(), Shape(),
                                          units::Unit(units::dimensionless))),
               except::UnitError);
  EXPECT_THROW(tan(createVariable<double>(Dims(), Shape(),
                                          units::Unit(units::dimensionless))),
               except::UnitError);
  EXPECT_THROW(
      asin(createVariable<double>(Dims(), Shape(), units::Unit(units::rad))),
      except::UnitError);
  EXPECT_THROW(
      acos(createVariable<double>(Dims(), Shape(), units::Unit(units::rad))),
      except::UnitError);
  EXPECT_THROW(
      atan(createVariable<double>(Dims(), Shape(), units::Unit(units::rad))),
      except::UnitError);
}
