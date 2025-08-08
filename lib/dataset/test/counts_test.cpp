// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(CountsTest, toDensity_fromDensity) {
  Dataset d({{"", makeVariable<double>(Dims{Dim::Time}, Shape{3},
                                       sc_units::counts, Values{12, 12, 12})}});
  d.setCoord(Dim::Time, makeVariable<double>(Dims{Dim::Time}, Shape{4},
                                             sc_units::us, Values{1, 2, 4, 8}));

  d = counts::toDensity(d, Dim::Time);
  auto result = d[""];
  EXPECT_EQ(result.unit(), sc_units::counts / sc_units::us);
  EXPECT_TRUE(equals(result.values<double>(), {12, 6, 3}));

  d = counts::fromDensity(d, Dim::Time);
  result = d[""];
  EXPECT_EQ(result.unit(), sc_units::counts);
  EXPECT_TRUE(equals(result.values<double>(), {12, 12, 12}));
}
