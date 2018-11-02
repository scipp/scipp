/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include <type_traits>

#include "dimensions.h"

TEST(Dimensions2, footprint) {
  EXPECT_EQ(sizeof(Dimensions2), 64);
  EXPECT_EQ(std::alignment_of<Dimensions2>(), 64);
}

TEST(Dimensions2, construct) {
  EXPECT_NO_THROW(Dimensions2());
  EXPECT_NO_THROW(Dimensions2{});
  EXPECT_NO_THROW((Dimensions2{Dim::X, 1}));
  EXPECT_NO_THROW((Dimensions2({Dim::X, 1})));
  EXPECT_NO_THROW((Dimensions2({{Dim::X, 1}, {Dim::Y, 1}})));
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
  dims.add(Dimension::Tof, 3);
  dims.add(Dimension::Q, 2);
  dims.erase(Dimension::Tof);
  EXPECT_FALSE(dims.contains(Dimension::Tof));
  EXPECT_TRUE(dims.contains(Dimension::Q));
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

TEST(Dimensions, merge) {
  Dimensions a;
  Dimensions b;
  a.add(Dimension::Tof, 3);
  EXPECT_EQ(merge(a, a).count(), 1);
  EXPECT_EQ(merge(a, b).count(), 1);
  EXPECT_EQ(merge(b, b).count(), 0);
  EXPECT_EQ(merge(merge(a, b), a).count(), 1);
  b.add(Dimension::Tof, 2);
  EXPECT_ANY_THROW(merge(a, b));
  b.resize(Dimension::Tof, 3);
  EXPECT_NO_THROW(merge(a, b));
}
