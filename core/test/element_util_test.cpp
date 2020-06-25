// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)

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
