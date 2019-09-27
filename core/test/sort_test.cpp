// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/core/sort.h"

using namespace scipp;
using namespace scipp::core;

TEST(SortTest, variable_1d) {
  const auto var =
      makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6});
  const auto key = makeVariable<int>({Dim::X, 3}, {10, 20, -1});
  const auto expected =
      makeVariable<int>({Dim::X, 3}, units::m, {3, 1, 2}, {6, 4, 5});

  EXPECT_EQ(sort(var, key), expected);
}

TEST(SortTest, variable_2d) {
  const auto var = makeVariable<int>({{Dim::Y, 2}, {Dim::X, 3}}, units::m,
                                     {1, 2, 3, 4, 5, 6});

  const auto keyX = makeVariable<int>({Dim::X, 3}, {10, 20, -1});
  const auto expectedX = makeVariable<int>({{Dim::Y, 2}, {Dim::X, 3}}, units::m,
                                           {3, 1, 2, 6, 4, 5});

  const auto keyY = makeVariable<int>({Dim::Y, 2}, {1, 0});
  const auto expectedY = makeVariable<int>({{Dim::Y, 2}, {Dim::X, 3}}, units::m,
                                           {4, 5, 6, 1, 2, 3});

  EXPECT_EQ(sort(var, keyX), expectedX);
  EXPECT_EQ(sort(var, keyY), expectedY);
}
