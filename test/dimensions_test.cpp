/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <type_traits>

#include "dimensions.h"

TEST(Dimensions, footprint) {
  EXPECT_EQ(sizeof(Dimensions), 64);
  EXPECT_EQ(std::alignment_of<Dimensions>(), 64);
}

TEST(Dimensions, construct) {
  EXPECT_NO_THROW(Dimensions());
  EXPECT_NO_THROW(Dimensions{});
  EXPECT_NO_THROW((Dimensions{Dim::X, 1}));
  EXPECT_NO_THROW((Dimensions({Dim::X, 1})));
  EXPECT_NO_THROW((Dimensions({{Dim::X, 1}, {Dim::Y, 1}})));
}

TEST(Dimensions, operator_equals) {
  EXPECT_EQ((Dimensions({Dim::X, 1})), (Dimensions({Dim::X, 1})));
}

TEST(Dimensions, count_and_volume) {
  Dimensions dims;
  EXPECT_EQ(dims.count(), 0);
  EXPECT_EQ(dims.volume(), 1);
  dims.add(Dimension::Tof, 3);
  EXPECT_EQ(dims.count(), 1);
  EXPECT_EQ(dims.volume(), 3);
  dims.add(Dimension::Q, 2);
  EXPECT_EQ(dims.count(), 2);
  EXPECT_EQ(dims.volume(), 6);
}

TEST(Dimensions, offset) {
  Dimensions dims;
  dims.add(Dimension::Tof, 3);
  dims.add(Dimension::Q, 2);
  EXPECT_EQ(dims.offset(Dimension::Tof), 1);
  EXPECT_EQ(dims.offset(Dimension::Q), 3);
}

TEST(Dimensions, erase) {
  Dimensions dims;
  dims.add(Dimension::X, 2);
  dims.add(Dimension::Y, 3);
  dims.add(Dimension::Z, 4);
  dims.erase(Dimension::Y);
  EXPECT_TRUE(dims.contains(Dimension::X));
  EXPECT_FALSE(dims.contains(Dimension::Y));
  EXPECT_TRUE(dims.contains(Dimension::Z));
  EXPECT_EQ(dims.volume(), 8);
}

TEST(Dimensions, erase_inner) {
  Dimensions dims;
  dims.add(Dimension::X, 2);
  dims.add(Dimension::Y, 3);
  dims.add(Dimension::Z, 4);
  dims.erase(Dimension::X);
  EXPECT_FALSE(dims.contains(Dimension::X));
  EXPECT_TRUE(dims.contains(Dimension::Y));
  EXPECT_TRUE(dims.contains(Dimension::Z));
  EXPECT_EQ(dims.volume(), 12);
}

TEST(Dimensions, contains_other) {
  Dimensions a;
  a.add(Dimension::Tof, 3);
  a.add(Dimension::Q, 2);

  EXPECT_TRUE(a.contains(Dimensions{}));
  EXPECT_TRUE(a.contains(a));
  EXPECT_TRUE(a.contains(Dimensions(Dimension::Q, 2)));
  EXPECT_FALSE(a.contains(Dimensions(Dimension::Q, 3)));

  Dimensions b;
  b.add(Dimension::Q, 2);
  b.add(Dimension::Tof, 3);
  // Order does not matter.
  EXPECT_TRUE(a.contains(b));
}

TEST(Dimensions, isContiguousIn) {
  Dimensions parent({{Dim::X, 4}, {Dim::Y, 3}, {Dim::Z, 2}});

  EXPECT_TRUE(parent.isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({Dim::X, 0}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 1}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 2}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 4}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({Dim::X, 5}).isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 0}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 1}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 2}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 3}}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::X, 4}, {Dim::Y, 4}}).isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 3}, {Dim::Z, 0}})
                  .isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 3}, {Dim::Z, 1}})
                  .isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::X, 4}, {Dim::Y, 3}, {Dim::Z, 2}})
                  .isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::X, 4}, {Dim::Y, 3}, {Dim::Z, 3}})
                   .isContiguousIn(parent));

  EXPECT_FALSE(Dimensions({Dim::Y, 3}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({Dim::Z, 2}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::X, 4}, {Dim::Z, 2}}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Y, 3}, {Dim::Z, 2}}).isContiguousIn(parent));
}
