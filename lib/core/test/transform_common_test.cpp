// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include "scipp/core/element/math.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

#include <gtest/gtest.h>
#include <vector>

using namespace scipp;
using namespace scipp::core;

TEST(TransformCommonTest, assign_unary_types) {
  auto aop_abs = assign_unary{core::element::abs};
  static_assert(std::is_same_v<decltype(aop_abs)::types,
                               decltype(core::element::abs)::types>);
  auto aop_sqrt = assign_unary{core::element::sqrt};
  static_assert(std::is_same_v<decltype(aop_sqrt)::types,
                               decltype(core::element::sqrt)::types>);
}

TEST(TransformCommonTest, assign_unary_value) {
  auto aop = assign_unary{core::element::abs};
  for (auto x : {54.2415698, -1.412, 0.0, 2.0}) {
    double y;
    aop(y, x);
    EXPECT_EQ(y, core::element::abs(x));
  }
}

TEST(TransformCommonTest, assign_unary_unit) {
  auto aop = assign_unary{core::element::sqrt};
  sc_units::Unit res;
  aop(res, sc_units::m * sc_units::m);
  EXPECT_EQ(res, sc_units::m);

  EXPECT_THROW(aop(res, sc_units::kg), except::UnitError);
}

TEST(TransformCommontest, assign_unary_value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  ValueAndVariance out(x);
  assign_unary{core::element::sqrt}(out, x);
  EXPECT_EQ(out, core::element::sqrt(x));
}
