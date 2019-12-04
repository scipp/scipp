// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

TEST(SumTest, masked_data_array) {
  const auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                                        {1.0, 2.0, 3.0, 4.0});
  const auto mask = makeVariable<bool>({Dim::X, 2}, {false, true});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto sumX = makeVariable<double>({Dim::Y, 2}, units::m, {1.0, 3.0});
  const auto sumY = makeVariable<double>({Dim::X, 2}, units::m, {4.0, 6.0});
  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
  EXPECT_FALSE(sum(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(sum(a, Dim::Y).masks().contains("mask"));
}
