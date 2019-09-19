// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

static const auto empty = Variable{};
static const auto scalar_double = makeVariable<double>(1.0);
static const auto scalar_int64 = makeVariable<int64_t>(1.0);

class VariableUnaryTest : public ::testing::Test,
                          public ::testing::WithParamInterface<Variable> {};

INSTANTIATE_TEST_SUITE_P(Dense, VariableUnaryTest,
                         ::testing::Values(empty, scalar_double, scalar_int64));
INSTANTIATE_TEST_SUITE_P(Sparse, VariableUnaryTest,
                         testing::Values(Variable()));

TEST_P(VariableUnaryTest, abs) {
  const auto var = GetParam();
  if (!var ||
      (var.dtype() != dtype<double> && var.dtype() != dtype<float> &&
       var.dtype() != dtype<int64_t> && var.dtype() != dtype<int32_t>)) {
    ASSERT_ANY_THROW(abs(var));
  } else {
    const auto result = abs(var);
    EXPECT_EQ(result.unit(), var.unit());
    EXPECT_EQ(result.dims(), var.dims());
    // how can we test values?
    // provide a map of expected result for each input?
    // what if there are multiple outputs, depending on other input params,
    // e.g., dimension for sum?
  }
}
