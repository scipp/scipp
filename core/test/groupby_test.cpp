// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/groupby.h"

using namespace scipp;
using namespace scipp::core;

TEST(GroupbyTest, dataset_1d) {
  Dataset d;
  d.setData("a",
            makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  d.setData("b", makeVariable<double>({Dim::X, 3}, units::s, {0.1, 0.2, 0.3}));
  d.setData("scalar", makeVariable<double>(1.2));
  d.setLabels("label1", makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}));
  d.setLabels("label2", makeVariable<double>({Dim::X, 3}, units::m, {1, 1, 3}));

  auto grouped = groupby(d, "label2");
  EXPECT_EQ(grouped.dims(), Dimensions({Dim::X, 2}));
  // Should we use Dim::Group instead, and coords()[Dim::Group]?
  // Or just hide the data array as an implementation detail (better!)
  const auto &keys = grouped.labels()["label2"].values<double>();
  EXPECT_EQ(keys[0], 1.0);
  EXPECT_EQ(keys[1], 3.0);
}
