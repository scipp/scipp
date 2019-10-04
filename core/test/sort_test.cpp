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

TEST(SortTest, dataset_1d) {
  Dataset d;
  d.setData("a",
            makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  d.setData("b", makeVariable<double>({Dim::X, 3}, units::s, {0.1, 0.2, 0.3}));
  d.setData("scalar", makeVariable<double>(1.2));
  d.setCoord(Dim::X, makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}));

  Dataset expected;
  expected.setData(
      "a", makeVariable<int>({Dim::X, 3}, units::m, {3, 1, 2}, {6, 4, 5}));
  expected.setData(
      "b", makeVariable<double>({Dim::X, 3}, units::s, {0.3, 0.1, 0.2}));
  expected.setCoord(Dim::X,
                    makeVariable<double>({Dim::X, 3}, units::m, {3, 1, 2}));

  const auto key = makeVariable<int>({Dim::X, 3}, {10, 20, -1});

  // Note that the result does not contain `scalar`. Is this a bug or a feature?
  // - Should we throw if there is any scalar data/coord?
  // - Should we preserve scalars?
  EXPECT_EQ(sort(d, key), expected);
}
