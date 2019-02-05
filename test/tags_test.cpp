/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "tags.h"

TEST(Tags, isDimensionCoord) {
  EXPECT_TRUE(isDimensionCoord[Coord::Tof.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::X.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::Y.value()]);
  EXPECT_TRUE(isDimensionCoord[Coord::Z.value()]);
  EXPECT_FALSE(isDimensionCoord[Coord::Position.value()]);
}
