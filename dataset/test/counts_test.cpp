// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(CountsTest, toDensity_fromDensity) {
  Dataset d;
  d.setCoord(Dim::Tof, makeVariable<double>(Dims{Dim::Tof}, Shape{4}, units::us,
                                            Values{1, 2, 4, 8}));
  d.setData("", makeVariable<double>(Dims{Dim::Tof}, Shape{3}, units::counts,
                                     Values{12, 12, 12}));

  d = counts::toDensity(std::move(d), Dim::Tof);
  auto result = d[""];
  EXPECT_EQ(result.unit(), units::counts / units::us);
  EXPECT_TRUE(equals(result.values<double>(), {12, 6, 3}));

  d = counts::fromDensity(std::move(d), Dim::Tof);
  result = d[""];
  EXPECT_EQ(result.unit(), units::counts);
  EXPECT_TRUE(equals(result.values<double>(), {12, 12, 12}));
}
