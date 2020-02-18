// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include "../element_unary_operations.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementSqrtTest, unit) {
  units::Unit m2(units::m * units::m);
  EXPECT_EQ(element::sqrt(m2), units::sqrt(m2));
}

TEST(ElementSqrtTest, value) {
  EXPECT_EQ(element::sqrt(1.23), std::sqrt(1.23));
  EXPECT_EQ(element::sqrt(1.23f), std::sqrt(1.23f));
}

TEST(ElementSqrtTest, value_and_variance) {
  ValueAndVariance x(2.0, 1.0);
  EXPECT_EQ(element::sqrt(x), sqrt(x));
}
