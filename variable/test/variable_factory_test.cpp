// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/variable_factory.h"

using namespace scipp;

TEST(VariableFactoryTest, values) {
  auto var = 0.0 * units::one;
  variable::variableFactory().values<double>(var)[0] = 1;
  EXPECT_EQ(var, 1.0 * units::one);
  variable::variableFactory().values<double>(Variable{var})[0] = 2;
  EXPECT_EQ(var, 2.0 * units::one);
}
