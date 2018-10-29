/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "tags.h"

TEST(Tags, isDimensionCoord) {
  EXPECT_TRUE(isDimensionCoord[tag_id<Coord::Tof>]);
  EXPECT_TRUE(isDimensionCoord[tag_id<Coord::X>]);
  EXPECT_TRUE(isDimensionCoord[tag_id<Coord::Y>]);
  EXPECT_TRUE(isDimensionCoord[tag_id<Coord::Z>]);
  EXPECT_FALSE(isDimensionCoord[tag_id<Coord::DetectorPosition>]);
}
