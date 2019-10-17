// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/groupby.h"

using namespace scipp;
using namespace scipp::core;

TEST(GroupbyTest, dataset_1d_and_2d) {
  Dataset d;
  d.setData("a",
            makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  d.setData("b", makeVariable<double>({Dim::X, 3}, units::s, {0.1, 0.2, 0.3}));
  d.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::X, 3}}, units::s,
                                      {1, 2, 3, 4, 5, 6}));
  d.setAttr("scalar", makeVariable<double>(1.2));
  d.setLabels("label1", makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}));
  d.setLabels("label2", makeVariable<double>({Dim::X, 3}, units::m, {1, 1, 3}));

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

  auto grouped = groupby(d, "label2", Dim::Y);
  EXPECT_EQ(grouped.mean(Dim::X), expected);
}
