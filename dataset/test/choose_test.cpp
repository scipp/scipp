// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/choose.h"

using namespace scipp;

TEST(ChooseTest, simple_1d) {
  const auto key = makeVariable<double>(Dims{Dim::Y}, Shape{5}, units::m,
                                        Values{2, 0, 0, 2, 2});
  DataArray choices(
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m, Values{11, 22, 33},
                           Variances{4, 5, 6}),
      {{Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m,
                                     Values{0, 2, 1})}});
  DataArray expected(makeVariable<double>(Dims{Dim::Y}, Shape{5}, units::m,
                                          Values{22, 11, 11, 22, 22},
                                          Variances{5, 4, 4, 5, 5}),
                     {{Dim::X, key}});

  EXPECT_EQ(choose(key, choices, Dim::X), expected);
}
