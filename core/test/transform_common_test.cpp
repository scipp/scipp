// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)

#include "scipp/core/transform_common.h"
#include "scipp/core/element/math.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"

#include <gtest/gtest.h>
#include <vector>

using namespace scipp;
using namespace scipp::core;

TEST(TransformCommonTest, assign_op_types) {
  auto aop_abs = assign_op{core::element::abs};
  static_assert(std::is_same_v<decltype(aop_abs)::types,
                               decltype(core::element::abs)::types>);
  auto aop_sqrt = assign_op{core::element::abs};
  static_assert(std::is_same_v<decltype(aop_sqrt)::types,
                               decltype(core::element::sqrt)::types>);
}

TEST(TransformCommonTest, assign_op_value) {
  auto aop = assign_op{core::element::abs};
  for (auto x : {54.2415698, -1.412, 0.0, 2.0}) {
    double y;
    aop(y, x);
    EXPECT_EQ(y, core::element::abs(x));
  }
}

TEST(TransformCommonTest, assign_op_unit) {
  auto aop = assign_op{core::element::sqrt};
  units::Unit res;
  aop(res, units::m*units::m);
  EXPECT_EQ(res, units::m);

  EXPECT_THROW(aop(res, units::kg), except::UnitError);
}

TEST(TransformCommontest, assign_op_value_and_variance) {
  const ValueAndVariance x(2.0, 1.0);
  ValueAndVariance out(x);
  assign_op{core::element::sqrt}(out, x);
  EXPECT_EQ(out, core::element::sqrt(x));
}
