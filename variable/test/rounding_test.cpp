// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/units/unit.h"
#include "scipp/variable/rounding.h"

TEST(RoundingTest, round) {
  auto preRoundedVar = makeVariable<double>(Dims{scipp::units::Dim::X},
                                            Values{1.2, 2.9, 1.5, 2.5});
  auto roundedVar =
      makeVariable<double>(Dims{scipp::units::Dim::X}, Values{1, 3, 2, 2});
  EXPECT_EQ(round(preRoundedVar), roundedVar);
}

TEST(RoundingTest, ceil) {
  auto preRoundedVar = makeVariable<double>(Dims{scipp::units::Dim::X},
                                            Values{1.2, 2.9, 1.5, 2.5});
  auto roundedVar =
      makeVariable<double>(Dims{scipp::units::Dim::X}, Values{2, 3, 2, 3});
  EXPECT_EQ(ceil(preRoundedVar), roundedVar);
}

TEST(RoundingTest, floor) {
  auto preRoundedVar = makeVariable<double>(Dims{scipp::units::Dim::X},
                                            Values{1.2, 2.9, 1.5, 2.5});
  auto roundedVar =
      makeVariable<double>(Dims{scipp::units::Dim::X}, Values{1, 2, 1, 2});
  EXPECT_EQ(floor(preRoundedVar), roundedVar);
}
