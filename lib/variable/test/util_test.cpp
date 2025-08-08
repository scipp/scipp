// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/units/unit.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

#include "test_macros.h"

using namespace scipp;

TEST(LinspaceTest, dim_mismatch) {
  EXPECT_THROW_DISCARD(
      linspace(1.0 * sc_units::one,
               makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::one),
               Dim::X, 4),
      except::DimensionError);
}

TEST(LinspaceTest, unit_mismatch) {
  EXPECT_THROW_DISCARD(
      linspace(1.0 * sc_units::one, 4.0 * sc_units::m, Dim::X, 4),
      except::UnitError);
}

TEST(LinspaceTest, dtype_mismatch) {
  EXPECT_THROW_DISCARD(
      linspace(1.0 * sc_units::one, 4.0f * sc_units::one, Dim::X, 4),
      except::TypeError);
}

TEST(LinspaceTest, non_float_fail) {
  EXPECT_THROW_DISCARD(
      linspace(1 * sc_units::one, 4 * sc_units::one, Dim::X, 4),
      except::TypeError);
}

TEST(LinspaceTest, variances_fail) {
  const auto a = 1.0 * sc_units::one;
  const auto b = makeVariable<double>(Values{1}, Variances{1});
  EXPECT_THROW_DISCARD(linspace(a, b, Dim::X, 4), except::VariancesError);
  EXPECT_THROW_DISCARD(linspace(b, a, Dim::X, 4), except::VariancesError);
  EXPECT_THROW_DISCARD(linspace(b, b, Dim::X, 4), except::VariancesError);
}

TEST(LinspaceTest, increasing) {
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  EXPECT_EQ(linspace(1.0 * sc_units::one, 4.0 * sc_units::one, Dim::X, 4),
            expected);
}

TEST(LinspaceTest, increasing_float) {
  const auto expected =
      makeVariable<float>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  EXPECT_EQ(linspace(1.0f * sc_units::one, 4.0f * sc_units::one, Dim::X, 4),
            expected);
}

TEST(LinspaceTest, with_unit) {
  const auto expected = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                             sc_units::m, Values{1, 2, 3, 4});
  EXPECT_EQ(linspace(1.0 * sc_units::m, 4.0 * sc_units::m, Dim::X, 4),
            expected);
}

TEST(LinspaceTest, fractional) {
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, sc_units::m,
                           Values{0.1, 0.1 + 0.1, 0.1 + 0.2, 0.4});
  EXPECT_EQ(linspace(0.1 * sc_units::m, 0.4 * sc_units::m, Dim::X, 4),
            expected);
}

TEST(LinspaceTest, decreasing) {
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{4, 3, 2, 1});
  EXPECT_EQ(linspace(4.0 * sc_units::one, 1.0 * sc_units::one, Dim::X, 4),
            expected);
}

TEST(LinspaceTest, increasing_2d) {
  const auto expected = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                             Values{1, 2, 3, 10, 20, 30});
  EXPECT_EQ(linspace(expected.slice({Dim::X, 0}), expected.slice({Dim::X, 2}),
                     Dim::X, 3),
            expected);
}

TEST(UtilTest, values_variances) {
  const auto var = makeVariable<double>(Values{1}, Variances{2}, sc_units::m);
  EXPECT_EQ(values(var), 1.0 * sc_units::m);
  EXPECT_EQ(variances(var), 2.0 * (sc_units::m * sc_units::m));
}

TEST(UtilTest, issorted_unknown_dim) {
  auto var = makeVariable<double>(Dims{Dim::X}, Values{1, 2, 3}, Shape{3});
  EXPECT_THROW_DISCARD(issorted(var, Dim::Y, SortOrder::Ascending),
                       except::DimensionError);
  var = makeVariable<double>(Values{1});
  EXPECT_THROW_DISCARD(issorted(var, Dim::Y, SortOrder::Ascending),
                       except::DimensionError);
}

TEST(UtilTest, issorted) {
  auto var =
      makeVariable<float>(Dimensions{{Dim::X, 3}, {Dim::Y, 3}}, sc_units::m,
                          Values{1, 2, 3, 1, 3, 2, 2, 2, 2});
  EXPECT_EQ(issorted(var.slice({Dim::Y, 1, 1}), Dim::X, SortOrder::Ascending),
            makeVariable<bool>(Dimensions{{Dim::Y, 0}}, Values{}));
  EXPECT_EQ(
      issorted(var, Dim::X, SortOrder::Ascending),
      makeVariable<bool>(Dimensions{{Dim::Y, 3}}, Values{true, false, false}));
  EXPECT_EQ(
      issorted(var, Dim::X, SortOrder::Descending),
      makeVariable<bool>(Dimensions{{Dim::Y, 3}}, Values{false, false, true}));
  EXPECT_EQ(
      issorted(var, Dim::Y, SortOrder::Ascending),
      makeVariable<bool>(Dimensions{{Dim::X, 3}}, Values{true, false, true}));
  EXPECT_EQ(
      issorted(var, Dim::Y, SortOrder::Descending),
      makeVariable<bool>(Dimensions{{Dim::X, 3}}, Values{false, false, true}));
}

TEST(UtilTest, issorted_small_dimensions) {
  auto var = makeVariable<float>(Dimensions{{Dim::X, 1}, {Dim::Y, 1}},
                                 sc_units::m, Values{1});
  EXPECT_EQ(issorted(var, Dim::X, SortOrder::Ascending),
            makeVariable<bool>(Dimensions{{Dim::Y, 1}}, Values{true}));
  EXPECT_EQ(issorted(var, Dim::X, SortOrder::Descending),
            makeVariable<bool>(Dimensions{{Dim::Y, 1}}, Values{true}));
  EXPECT_EQ(issorted(var, Dim::Y, SortOrder::Ascending),
            makeVariable<bool>(Dimensions{{Dim::X, 1}}, Values{true}));
  EXPECT_EQ(issorted(var, Dim::Y, SortOrder::Descending),
            makeVariable<bool>(Dimensions{{Dim::X, 1}}, Values{true}));
}

TEST(UtilTest, allsorted_single_dimension_ascending) {
  auto var = makeVariable<double>(Dims{Dim::X}, Values{1, 2, 3}, Shape{3});
  EXPECT_TRUE(allsorted(var, Dim::X, SortOrder::Ascending));
  EXPECT_FALSE(allsorted(var, Dim::X, SortOrder::Descending));
}

TEST(UtilTest, allsorted_single_dimension_descending) {
  auto var = makeVariable<double>(Dims{Dim::X}, Values{3, 2, 1}, Shape{3});
  EXPECT_FALSE(allsorted(var, Dim::X, SortOrder::Ascending));
  EXPECT_TRUE(allsorted(var, Dim::X, SortOrder::Descending));
}

TEST(UtilTest, allsorted_multidimensional) {
  auto var = makeVariable<float>(Dimensions{{Dim::X, 2}, {Dim::Y, 2}},
                                 Values{1, 2, 0, 1});
  EXPECT_TRUE(allsorted(var, Dim::Y, SortOrder::Ascending));
  EXPECT_FALSE(allsorted(var, Dim::X, SortOrder::Ascending));
  EXPECT_FALSE(allsorted(var, Dim::Y, SortOrder::Descending));
  EXPECT_TRUE(allsorted(var, Dim::X, SortOrder::Descending));
}

TEST(VariableTest, where) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                  Values{1, 2, 3});
  auto mask =
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{true, false, true});
  auto expected_var = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{1, 4, 3});
  EXPECT_EQ(where(mask, var, var + var), expected_var);
}
