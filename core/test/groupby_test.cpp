// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/groupby.h"

using namespace scipp;
using namespace scipp::core;

TEST(GroupbyTest, dataset_1d_and_2d) {
  Dataset d;
  d.setData("a",
            makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  d.setData("b", makeVariable<double>({Dim::X, 3}, units::s, {0.1, 0.2, 0.3}));
  d.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::X, 3}}, units::s,
                                      {1, 2, 3, 4, 5, 6}));
  d.setAttr("scalar", makeVariable<double>(1.2));
  d.setLabels("labels1",
              makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}));
  d.setLabels("labels2",
              makeVariable<double>({Dim::X, 3}, units::m, {1, 1, 3}));

  Dataset expected;
  expected.setData("a", makeVariable<double>({Dim::Y, 2}, units::m, {1.5, 3.0},
                                             {9.0 / 4, 6.0}));
  expected.setData("b", makeVariable<double>({Dim::Y, 2}, units::s,
                                             {(0.1 + 0.2) / 2.0, 0.3}));
  expected.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::Y, 2}},
                                             units::s, {1.5, 3.0, 4.5, 6.0}));
  expected.setAttr("scalar", makeVariable<double>(1.2));
  expected.setCoord(Dim::Y,
                    makeVariable<double>({Dim::Y, 2}, units::m, {1, 3}));

  auto grouped = groupby(d, "labels2", Dim::Y);
  EXPECT_EQ(grouped.mean(Dim::X), expected);
}

TEST(GroupbyTest, bins) {
  Dataset d;
  d.setData("a", makeVariable<double>({Dim::X, 5}, units::s,
                                      {0.1, 0.2, 0.3, 0.4, 0.5}));
  d.setData("b", makeVariable<double>({{Dim::Y, 2}, {Dim::X, 5}}, units::s,
                                      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
  d.setAttr("scalar", makeVariable<double>(1.2));
  d.setLabels("labels1",
              makeVariable<double>({Dim::X, 5}, units::m, {1, 2, 3, 4, 5}));
  d.setLabels("labels2", makeVariable<double>({Dim::X, 5}, units::m,
                                              {1.0, 1.1, 2.5, 4.0, 1.2}));

  auto bins = makeVariable<double>({Dim::Z, 4}, units::m, {0.0, 1.0, 2.0, 3.0});

  Dataset expected;
  expected.setCoord(Dim::Z, bins);
  expected.setData(
      "a", makeVariable<double>({Dim::Z, 3}, units::s, {0.0, 0.8, 0.3}));
  expected.setData("b", makeVariable<double>({{Dim::Y, 2}, {Dim::Z, 3}},
                                             units::s, {0, 8, 3, 0, 23, 8}));
  expected.setAttr("scalar", makeVariable<double>(1.2));

  EXPECT_EQ(groupby(d, "labels2", bins).sum(Dim::X), expected);
}
