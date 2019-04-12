// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "tags.h"

using namespace scipp::core;

TEST(Tags, isDimensionCoord) {
  EXPECT_TRUE(isDimensionCoord[Coord::Tof.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::X.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::Y.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::Z.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::Position.value()]);
  EXPECT_FALSE(isDimensionCoord[Coord::Mask.value()]);
}
