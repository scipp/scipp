// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

#include "scipp/core/element/util.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

#include <cmath>
#include <gtest/gtest.h>
#include <limits>
#include <vector>

using namespace scipp;
using namespace scipp::core::element;

TEST(ElementUtilTest, convertMaskedToZero_masks_special_vals) {
  EXPECT_EQ(convertMaskedToZero(1.0, true), 0.0);
  EXPECT_EQ(convertMaskedToZero(std::numeric_limits<double>::quiet_NaN(), true),
            0.0);
  EXPECT_EQ(convertMaskedToZero(std::numeric_limits<double>::infinity(), true),
            0.0);
  EXPECT_EQ(convertMaskedToZero(1.0, false), 1.0);
}

TEST(ElementUtilTest, convertMaskedToZero_ignores_unmasked) {
  EXPECT_TRUE(std::isnan(
      convertMaskedToZero(std::numeric_limits<double>::quiet_NaN(), false)));

  EXPECT_TRUE(std::isinf(
      convertMaskedToZero(std::numeric_limits<double>::infinity(), false)));
}

TEST(ElementUtilTest, convertMaskedToZero_handles_units) {
  const auto dimensionless = scipp::units::dimensionless;

  for (const auto &unit :
       {scipp::units::m, scipp::units::dimensionless, scipp::units::s}) {
    // Unit 'a' should always be preserved
    EXPECT_EQ(convertMaskedToZero(unit, dimensionless), unit);
  }
}

TEST(ElementUtilTest, convertMaskedToZero_rejects_units_with_dim) {
  const auto seconds = scipp::units::s;

  for (const auto &unit :
       {scipp::units::m, scipp::units::kg, scipp::units::s}) {
    // Unit 'b' should always be dimensionless as its a mask
    EXPECT_THROW(convertMaskedToZero(seconds, unit), scipp::except::UnitError);
  }
}

TEST(ElementUtilTest, convertMaskedToZero_accepts_all_types) {
  static_assert(
      std::is_same_v<decltype(convertMaskedToZero(bool{}, true)), bool>);
  static_assert(
      std::is_same_v<decltype(convertMaskedToZero(double{}, true)), double>);
  static_assert(
      std::is_same_v<decltype(convertMaskedToZero(float{}, true)), float>);
  static_assert(
      std::is_same_v<decltype(convertMaskedToZero(int32_t{}, true)), int32_t>);
  static_assert(
      std::is_same_v<decltype(convertMaskedToZero(int64_t{}, true)), int64_t>);
}

TEST(ElementUtilTest, values_variances) {
  ValueAndVariance x{1.0, 2.0};
  EXPECT_EQ(values(units::m), units::m);
  EXPECT_EQ(values(x), 1.0);
  EXPECT_EQ(values(1.2), 1.2);
  EXPECT_EQ(variances(units::m), units::m * units::m);
  EXPECT_EQ(variances(x), 2.0);
}

namespace {
constexpr auto test_is_sorted = [](const auto sorted, const bool order) {
  const auto expect_sorted_eq = [&sorted](const auto a, const auto b,
                                          const auto expected) {
    bool out = true;
    sorted(out, a, b);
    EXPECT_EQ(out, expected);
  };
  expect_sorted_eq(1.0, 2.0, order);
  expect_sorted_eq(-1.0, 1.0, order);
  expect_sorted_eq(-2.0, -1.0, order);
  expect_sorted_eq(1.0, 1.0, true);
  expect_sorted_eq(2.0, 1.0, !order);
  expect_sorted_eq(1.0, -1.0, !order);
  expect_sorted_eq(-1.0, -2.0, !order);
  units::Unit unit = units::one;
  sorted(unit, units::m, units::m);
  EXPECT_EQ(unit, units::one);
  EXPECT_THROW(sorted(unit, units::m, units::s), except::UnitError);
};
}

TEST(ElementUtilTest, is_sorted) {
  test_is_sorted(is_sorted_nondescending, true);
  test_is_sorted(is_sorted_nonascending, false);
}

TEST(ElementUtilTest, zip) {
  EXPECT_EQ(zip(1, 2), (std::pair{1, 2}));
  EXPECT_EQ(zip(3, 4), (std::pair{3, 4}));
  EXPECT_EQ(zip(units::m, units::m), units::m);
  EXPECT_EQ(zip(units::s, units::s), units::s);
  EXPECT_THROW(zip(units::m, units::s), except::UnitError);
}

TEST(ElementUtilTest, get) {
  EXPECT_EQ(core::element::get<0>(std::pair{1, 2}), 1);
  EXPECT_EQ(core::element::get<1>(std::pair{1, 2}), 2);
  EXPECT_EQ(core::element::get<0>(std::pair{3, 4}), 3);
  EXPECT_EQ(core::element::get<1>(std::pair{3, 4}), 4);
  EXPECT_EQ(core::element::get<0>(units::m), units::m);
  EXPECT_EQ(core::element::get<0>(units::s), units::s);
  EXPECT_EQ(core::element::get<1>(units::m), units::m);
  EXPECT_EQ(core::element::get<1>(units::s), units::s);
}

TEST(ElementUtilTest, fill) {
  double f64;
  float f32;
  ValueAndVariance x{1.0, 2.0};
  units::Unit u;
  fill(f64, 4.5);
  EXPECT_EQ(f64, 4.5);
  fill(f32, 4.5);
  EXPECT_EQ(f32, 4.5);
  fill(x, 4.5);
  EXPECT_EQ(x, (ValueAndVariance{4.5, 0.0}));
  fill(x, ValueAndVariance{1.2, 3.4});
  EXPECT_EQ(x, (ValueAndVariance{1.2, 3.4}));
  fill(u, units::m);
  EXPECT_EQ(u, units::m);
}

TEST(ElementUtilTest, fill_zeros) {
  double x = 1.2;
  ValueAndVariance y{1.0, 2.0};
  units::Unit u = units::m;
  fill_zeros(x);
  EXPECT_EQ(x, 0.0);
  fill_zeros(y);
  EXPECT_EQ(y, (ValueAndVariance{0.0, 0.0}));
  fill_zeros(u);
  EXPECT_EQ(u, units::m); // unchanged
}
