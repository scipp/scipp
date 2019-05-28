// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "dataset.h"
#include "scipp/core/counts.h"

using namespace scipp;
using namespace scipp::core;

TEST(CountsTest, toDensity_fromDensity) {
  Dataset d;
  d.setCoord(Dim::Tof,
             makeVariable<double>({Dim::Tof, 4}, units::us, {1, 2, 4, 8}));
  d.setData("",
            makeVariable<double>({Dim::Tof, 3}, units::counts, {12, 12, 12}));

  d = counts::toDensity(std::move(d), Dim::Tof);
  auto result = d[""];
  EXPECT_EQ(result.unit(), units::counts / units::us);
  EXPECT_TRUE(equals(result.values<double>(), {12, 6, 3}));

  d = counts::fromDensity(std::move(d), Dim::Tof);
  result = d[""];
  EXPECT_EQ(result.unit(), units::counts);
  EXPECT_TRUE(equals(result.values<double>(), {12, 12, 12}));
}
