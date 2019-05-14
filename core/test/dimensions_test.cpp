// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "dimensions.h"

using namespace scipp::core;

TEST(Dimensions, footprint) {
  EXPECT_EQ(sizeof(Dimensions), 64ul);
  // TODO Do we want to align this? Need to run benchmarks when implementation
  // is more mature.
  // EXPECT_EQ(std::alignment_of<Dimensions>(), 64);
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
  dims.add(Dim::Tof, 3);
  EXPECT_EQ(dims.count(), 1);
  EXPECT_EQ(dims.volume(), 3);
  dims.add(Dim::Q, 2);
  EXPECT_EQ(dims.count(), 2);
  EXPECT_EQ(dims.volume(), 6);
}

TEST(Dimensions, offset_from_list_init) {
  // Leftmost is outer dimension, rightmost is inner dimension.
  Dimensions dims{{Dim::Q, 2}, {Dim::Tof, 3}};
  EXPECT_EQ(dims.offset(Dim::Tof), 1);
  EXPECT_EQ(dims.offset(Dim::Q), 3);
}

TEST(Dimensions, offset) {
  Dimensions dims;
  dims.add(Dim::Tof, 3);
  dims.add(Dim::Q, 2);
  EXPECT_EQ(dims.offset(Dim::Tof), 1);
  EXPECT_EQ(dims.offset(Dim::Q), 3);
}

TEST(Dimensions, erase) {
  Dimensions dims;
  dims.add(Dim::X, 2);
  dims.add(Dim::Y, 3);
  dims.add(Dim::Z, 4);
  dims.erase(Dim::Y);
  EXPECT_TRUE(dims.contains(Dim::X));
  EXPECT_FALSE(dims.contains(Dim::Y));
  EXPECT_TRUE(dims.contains(Dim::Z));
  EXPECT_EQ(dims.volume(), 8);
  EXPECT_EQ(dims.index(Dim::Z), 0);
  EXPECT_EQ(dims.index(Dim::X), 1);
}

TEST(Dimensions, erase_inner) {
  Dimensions dims;
  dims.add(Dim::X, 2);
  dims.add(Dim::Y, 3);
  dims.add(Dim::Z, 4);
  dims.erase(Dim::X);
  EXPECT_FALSE(dims.contains(Dim::X));
  EXPECT_TRUE(dims.contains(Dim::Y));
  EXPECT_TRUE(dims.contains(Dim::Z));
  EXPECT_EQ(dims.volume(), 12);
}

TEST(Dimensions, contains_other) {
  Dimensions a;
  a.add(Dim::Tof, 3);
  a.add(Dim::Q, 2);

  EXPECT_TRUE(a.contains(Dimensions{}));
  EXPECT_TRUE(a.contains(a));
  EXPECT_TRUE(a.contains(Dimensions(Dim::Q, 2)));
  EXPECT_FALSE(a.contains(Dimensions(Dim::Q, 3)));

  Dimensions b;
  b.add(Dim::Q, 2);
  b.add(Dim::Tof, 3);
  // Order does not matter.
  EXPECT_TRUE(a.contains(b));
}

TEST(Dimensions, isContiguousIn) {
  Dimensions parent({{Dim::Z, 2}, {Dim::Y, 3}, {Dim::X, 4}});

  EXPECT_TRUE(parent.isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({Dim::X, 0}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 1}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 2}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({Dim::X, 4}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({Dim::X, 5}).isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({{Dim::Y, 0}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Y, 1}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Y, 2}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Y, 3}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Y, 4}, {Dim::X, 4}}).isContiguousIn(parent));

  EXPECT_TRUE(Dimensions({{Dim::Z, 0}, {Dim::Y, 3}, {Dim::X, 4}})
                  .isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Z, 1}, {Dim::Y, 3}, {Dim::X, 4}})
                  .isContiguousIn(parent));
  EXPECT_TRUE(Dimensions({{Dim::Z, 2}, {Dim::Y, 3}, {Dim::X, 4}})
                  .isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Z, 3}, {Dim::Y, 3}, {Dim::X, 4}})
                   .isContiguousIn(parent));

  EXPECT_FALSE(Dimensions({Dim::Y, 3}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({Dim::Z, 2}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Z, 2}, {Dim::X, 4}}).isContiguousIn(parent));
  EXPECT_FALSE(Dimensions({{Dim::Z, 2}, {Dim::Y, 3}}).isContiguousIn(parent));
}

TEST(DimensionsTest, isSparse) {
  Dimensions denseXY({Dim::X, Dim::Y}, {2, 3});
  Dimensions denseXYZ({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});
  Dimensions sparseXYZ({Dim::X, Dim::Y, Dim::Z}, {2, 3});

  EXPECT_FALSE(denseXY.isSparse());
  EXPECT_FALSE(denseXYZ.isSparse());
  EXPECT_TRUE(sparseXYZ.isSparse());
}

class DimensionsTest_comparison_operators : public ::testing::Test {
protected:
  void expect_eq(const Dimensions &a, const Dimensions &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
  }
  void expect_ne(const Dimensions &a, const Dimensions &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }
};

TEST_F(DimensionsTest_comparison_operators, sparse) {
  Dimensions dense_xy({Dim::X, Dim::Y}, {2, 3});
  Dimensions dense_xyz({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});
  Dimensions sparse_xyz({Dim::X, Dim::Y, Dim::Z}, {2, 3});
  Dimensions sparse_yxz({Dim::Y, Dim::X, Dim::Z}, {2, 3});
  Dimensions sparse_xyr({Dim::X, Dim::Y, Dim::Row}, {2, 3});

  expect_eq(sparse_xyz, sparse_xyz);
  expect_ne(sparse_xyz, sparse_yxz);
  expect_ne(sparse_xyz, sparse_xyr);
  expect_ne(sparse_xyz, dense_xy);
  expect_ne(sparse_xyz, dense_xyz);
}
