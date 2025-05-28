// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/hyperbolic.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/variable_factory.h"

#include "test_macros.h"
#include "test_variables.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::sc_units;

TEST(VariableHyperbolicTest, sinh) {
  const auto input = makeVariable<double>(Dims{}, Values{0.5});
  const auto var = copy(input);
  const auto expected = makeVariable<double>(Dims{}, Values{std::sinh(0.5)});
  EXPECT_EQ(sinh(var), expected);
  EXPECT_EQ(var, input);
}

TEST(VariableHyperbolicTest, sinh_out_arg) {
  const auto input = makeVariable<double>(Dims{}, Values{0.5});
  const auto var = copy(input);
  auto output = special_like(input, FillValue::ZeroNotBool);
  const auto expected = makeVariable<double>(Dims{}, Values{std::sinh(0.5)});

  auto &view = sinh(var, output);
  EXPECT_EQ(output, expected);
  EXPECT_EQ(&view, &output);
  EXPECT_EQ(var, input);
}

TEST(VariableHyperbolicTest, acosh) {
  const auto input = makeVariable<double>(Dims{}, Values{1.5});
  const auto var = copy(input);
  const auto expected = makeVariable<double>(Dims{}, Values{std::acosh(1.5)});
  EXPECT_EQ(acosh(var), expected);
  EXPECT_EQ(var, input);
}

TEST(VariableHyperbolicTest, acosh_out_arg) {
  const auto input = makeVariable<double>(Dims{}, Values{1.5});
  const auto var = copy(input);
  auto output = special_like(input, FillValue::ZeroNotBool);
  const auto expected = makeVariable<double>(Dims{}, Values{std::acosh(1.5)});

  auto &view = acosh(var, output);
  EXPECT_EQ(output, expected);
  EXPECT_EQ(&view, &output);
  EXPECT_EQ(var, input);
}
