// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(VariableTrigonometryTest, sin) {
  const auto rad = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{pi<double>});
  const auto deg = makeVariable<double>(Dims(), Shape(),
                                        units::Unit(units::deg), Values{180.0});

  const auto expected =
      makeVariable<double>(Dims(), Shape(), units::Unit(units::dimensionless),
                           Values{sin(pi<double>)});

  EXPECT_EQ(sin(rad), expected);
  EXPECT_EQ(sin(deg), expected);
}

TEST(VariableTrigonometryTest, sin_in_place_full) {
  auto rad =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::Unit(units::rad),
                           Values{0.0, pi<double>, 2.0 * pi<double>});
  auto deg =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::Unit(units::deg),
                           Values{0.0, 180.0, 360.0});

  auto rad_view = sin(rad, rad);
  auto deg_view = sin(deg, deg);

  const auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{3},
      Values{sin(0.0), sin(pi<double>), sin(2.0 * pi<double>)});

  EXPECT_EQ(rad, expected);
  EXPECT_EQ(rad_view, rad);
  EXPECT_EQ(rad_view.underlying(), rad);

  EXPECT_EQ(deg, expected);
  EXPECT_EQ(deg_view, deg);
  EXPECT_EQ(deg_view.underlying(), deg);
}

TEST(VariableTrigonometryTest, sin_in_place_partial) {
  const auto rad =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::Unit(units::rad),
                           Values{0.0, pi<double>, 2.0 * pi<double>});
  const auto deg =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::Unit(units::deg),
                           Values{0.0, 180.0, 360.0});

  auto rad_out = makeVariable<double>(Dims{Dim::X}, Shape{2});
  auto deg_out = makeVariable<double>(Dims{Dim::X}, Shape{2});

  auto rad_view = sin(rad.slice({Dim::X, 1, 3}), rad_out);
  auto deg_view = sin(rad.slice({Dim::X, 1, 3}), deg_out);

  const auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{sin(pi<double>), sin(2.0 * pi<double>)});

  EXPECT_EQ(rad_out, expected);
  EXPECT_EQ(rad_view, rad_out);
  EXPECT_EQ(rad_view.underlying(), rad_out);

  EXPECT_EQ(deg_out, expected);
  EXPECT_EQ(deg_view, deg_out);
  EXPECT_EQ(deg_view.underlying(), deg_out);
}

TEST(VariableTrigonometryTest, cos) {
  const auto rad = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{pi<double>});
  const auto deg = makeVariable<double>(Dims(), Shape(),
                                        units::Unit(units::deg), Values{180.0});
  const auto expected =
      makeVariable<double>(Dims(), Shape(), units::Unit(units::dimensionless),
                           Values{cos(pi<double>)});
  EXPECT_EQ(cos(rad), expected);
  EXPECT_EQ(cos(deg), expected);
}

TEST(VariableTrigonometryTest, tan) {
  const auto rad = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{pi<double>});
  const auto deg = makeVariable<double>(Dims(), Shape(),
                                        units::Unit(units::deg), Values{180.0});
  const auto expected =
      makeVariable<double>(Dims(), Shape(), units::Unit(units::dimensionless),
                           Values{tan(pi<double>)});
  EXPECT_EQ(tan(rad), expected);
  EXPECT_EQ(tan(deg), expected);
}

TEST(VariableTrigonometryTest, asin) {
  const auto var = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::dimensionless), Values{1.0});
  const auto expected = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{0.5 * pi<double>});
  EXPECT_EQ(asin(var), expected);
}

TEST(VariableTrigonometryTest, acos) {
  const auto var = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::dimensionless), Values{1.0});
  const auto expected = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{0.0});
  EXPECT_EQ(acos(var), expected);
}

TEST(VariableTrigonometryTest, atan) {
  const auto var = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::dimensionless), Values{1.0});
  const auto expected = makeVariable<double>(
      Dims(), Shape(), units::Unit(units::rad), Values{0.25 * pi<double>});
  EXPECT_EQ(atan(var), expected);
}

TEST(VariableTrigonometryTest, unit_fail) {
  EXPECT_THROW(sin(makeVariable<double>(Dims(), Shape(),
                                        units::Unit(units::dimensionless))),
               except::UnitError);
  EXPECT_THROW(cos(makeVariable<double>(Dims(), Shape(),
                                        units::Unit(units::dimensionless))),
               except::UnitError);
  EXPECT_THROW(tan(makeVariable<double>(Dims(), Shape(),
                                        units::Unit(units::dimensionless))),
               except::UnitError);
  EXPECT_THROW(
      asin(makeVariable<double>(Dims(), Shape(), units::Unit(units::rad))),
      except::UnitError);
  EXPECT_THROW(
      acos(makeVariable<double>(Dims(), Shape(), units::Unit(units::rad))),
      except::UnitError);
  EXPECT_THROW(
      atan(makeVariable<double>(Dims(), Shape(), units::Unit(units::rad))),
      except::UnitError);
}
