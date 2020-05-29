// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/reduction.h"
#include "scipp/core/except.h"

using namespace scipp;
using namespace scipp::core::element;

TEST(ReductionTest, flatten_unit) {
  auto u = units::m;
  flatten(u, units::m, units::one);
  EXPECT_EQ(u, units::m);
  EXPECT_THROW(flatten(u, units::s, units::one), except::UnitError);
  EXPECT_THROW(flatten(u, units::m, units::m), except::UnitError);
}

TEST(ReductionTest, flatten) {
  const std::vector a{1, 2};
  const std::vector b{3, 4};
  auto out = a;
  flatten(out, b, false);
  EXPECT_EQ(out, a);
  flatten(out, b, true);
  EXPECT_EQ(out, std::vector({1, 2, 3, 4}));
}
