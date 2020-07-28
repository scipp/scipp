// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/util.h"

using namespace scipp;

TEST(LinspaceTest, dim_mismatch) {
  EXPECT_THROW(
      linspace(1.0 * units::one,
               makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::one), Dim::X,
               4),
      except::DimensionMismatchError);
}

TEST(LinspaceTest, unit_mismatch) {
  EXPECT_THROW(linspace(1.0 * units::one, 4.0 * units::m, Dim::X, 4),
               except::UnitMismatchError);
}

TEST(LinspaceTest, dtype_mismatch) {
  EXPECT_THROW(linspace(1.0 * units::one, 4.0f * units::one, Dim::X, 4),
               except::TypeMismatchError);
}

TEST(LinspaceTest, non_float_fail) {
  EXPECT_THROW(linspace(1 * units::one, 4 * units::one, Dim::X, 4),
               except::TypeError);
}

TEST(LinspaceTest, variances_fail) {
  const auto a = 1.0 * units::one;
  const auto b = makeVariable<double>(Values{1}, Variances{1});
  EXPECT_THROW(linspace(a, b, Dim::X, 4), except::VariancesError);
  EXPECT_THROW(linspace(b, a, Dim::X, 4), except::VariancesError);
  EXPECT_THROW(linspace(b, b, Dim::X, 4), except::VariancesError);
}

TEST(LinspaceTest, increasing) {
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  EXPECT_EQ(linspace(1.0 * units::one, 4.0 * units::one, Dim::X, 4), expected);
}

TEST(LinspaceTest, increasing_float) {
  const auto expected =
      makeVariable<float>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  EXPECT_EQ(linspace(1.0f * units::one, 4.0f * units::one, Dim::X, 4),
            expected);
}

TEST(LinspaceTest, with_unit) {
  const auto expected = makeVariable<double>(Dims{Dim::X}, Shape{4}, units::m,
                                             Values{1, 2, 3, 4});
  EXPECT_EQ(linspace(1.0 * units::m, 4.0 * units::m, Dim::X, 4), expected);
}

TEST(LinspaceTest, fractional) {
  const auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, units::m, Values{0.1, 0.1 + 0.1, 0.1 + 0.2, 0.4});
  EXPECT_EQ(linspace(0.1 * units::m, 0.4 * units::m, Dim::X, 4), expected);
}

TEST(LinspaceTest, decreasing) {
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{4, 3, 2, 1});
  EXPECT_EQ(linspace(4.0 * units::one, 1.0 * units::one, Dim::X, 4), expected);
}

TEST(LinspaceTest, increasing_2d) {
  const auto expected = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                             Values{1, 2, 3, 10, 20, 30});
  EXPECT_EQ(linspace(expected.slice({Dim::X, 0}), expected.slice({Dim::X, 2}),
                     Dim::X, 3),
            expected);
}

TEST(UtilTest, values_variances) {
  const auto var = makeVariable<double>(Values{1}, Variances{2}, units::m);
  EXPECT_EQ(values(var), 1.0 * units::m);
  EXPECT_EQ(variances(var), 2.0 * (units::m * units::m));
}
