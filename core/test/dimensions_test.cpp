// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"

using namespace scipp;
using namespace scipp::core;

TEST(DimensionsTest, footprint) {
  EXPECT_EQ(sizeof(Dimensions), 64ul);
  // TODO Do we want to align this? Need to run benchmarks when implementation
  // is more mature.
  // EXPECT_EQ(std::alignment_of<Dimensions>(), 64);
}

TEST(DimensionsTest, construct) {
  EXPECT_NO_THROW(Dimensions());
  EXPECT_NO_THROW(Dimensions{});
  EXPECT_NO_THROW((Dimensions{Dim::X, 1}));
  EXPECT_NO_THROW((Dimensions({Dim::X, 1})));
  EXPECT_NO_THROW((Dimensions({{Dim::X, 1}, {Dim::Y, 1}})));
}

TEST(DimensionsTest, count_and_volume) {
  Dimensions dims;
  EXPECT_EQ(dims.shape().size(), 0);
  EXPECT_EQ(dims.volume(), 1);
  dims.add(Dim::Tof, 3);
  EXPECT_EQ(dims.shape().size(), 1);
  EXPECT_EQ(dims.volume(), 3);
  dims.add(Dim::Q, 2);
  EXPECT_EQ(dims.shape().size(), 2);
  EXPECT_EQ(dims.volume(), 6);
}

TEST(DimensionsTest, offset_from_list_init) {
  // Leftmost is outer dimension, rightmost is inner dimension.
  Dimensions dims{{Dim::Q, 2}, {Dim::Tof, 3}};
  EXPECT_EQ(dims.offset(Dim::Tof), 1);
  EXPECT_EQ(dims.offset(Dim::Q), 3);
}

TEST(DimensionsTest, offset) {
  Dimensions dims;
  dims.add(Dim::Tof, 3);
  dims.add(Dim::Q, 2);
  EXPECT_EQ(dims.offset(Dim::Tof), 1);
  EXPECT_EQ(dims.offset(Dim::Q), 3);
}

TEST(DimensionsTest, erase) {
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

TEST(DimensionsTest, erase_inner) {
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

TEST(DimensionsTest, contains_other) {
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

TEST(DimensionsTest, isContiguousIn) {
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

TEST(DimensionsTest, index_access) {
  Dimensions denseXY({Dim::X, Dim::Y}, {2, 3});
  Dimensions denseXYZ({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});

  ASSERT_THROW(denseXY[Dim::Invalid], except::DimensionNotFoundError);
  ASSERT_THROW(denseXYZ[Dim::Invalid], except::DimensionNotFoundError);
  ASSERT_THROW(denseXY[Dim::Z], except::DimensionNotFoundError);
  ASSERT_NO_THROW(denseXYZ[Dim::Z]);
}

TEST(DimensionsTest, duplicate) {
  Dimensions dense({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});

  ASSERT_THROW(dense.add(Dim::X, 2), except::DimensionError);
  ASSERT_THROW(dense.add(Dim::Y, 2), except::DimensionError);
  ASSERT_THROW(dense.add(Dim::Z, 2), except::DimensionError);
  ASSERT_THROW(dense.addInner(Dim::X, 2), except::DimensionError);
  ASSERT_THROW(dense.addInner(Dim::Y, 2), except::DimensionError);
  ASSERT_THROW(dense.addInner(Dim::Z, 2), except::DimensionError);
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

TEST_F(DimensionsTest_comparison_operators, dense_0d) {
  Dimensions empty;
  expect_eq(empty, empty);
}

TEST_F(DimensionsTest_comparison_operators, dense_1d) {
  Dimensions empty;
  Dimensions x2(Dim::X, 2);
  Dimensions x3(Dim::X, 3);
  Dimensions y2(Dim::Y, 2);

  expect_eq(x2, x2);
  expect_ne(x2, empty);
  expect_ne(x2, x3);
  expect_ne(x2, y2);
}

TEST_F(DimensionsTest_comparison_operators, dense_2d) {
  Dimensions x2(Dim::X, 2);
  Dimensions x2y3({Dim::X, Dim::Y}, {2, 3});
  Dimensions y3x2({Dim::Y, Dim::X}, {3, 2});
  Dimensions x3y2({Dim::X, Dim::Y}, {3, 2});

  expect_eq(x2y3, x2y3);
  expect_ne(x2y3, x2);
  expect_ne(x2y3, y3x2);
  expect_ne(x2y3, x3y2);
}

TEST(DimensionsTest, merge_self) {
  Dimensions dims({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});
  EXPECT_EQ(merge(dims, dims), dims);
}

TEST(DimensionsTest, merge_dense) {
  Dimensions a(Dim::X, 2);
  Dimensions b({Dim::Y, Dim::Z}, {3, 4});
  EXPECT_EQ(merge(a, b), Dimensions({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}));
}

TEST(DimensionsTest, merge_dense_overlapping) {
  Dimensions a({Dim::X, Dim::Y}, {2, 3});
  Dimensions b({Dim::Y, Dim::Z}, {3, 4});
  EXPECT_EQ(merge(a, b), Dimensions({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}));
}

TEST(DimensionsTest, merge_dense_different_order) {
  // The current implementation "favors" the order of the first argument if both
  // inputs have the same number of dimension, but this is not necessarily a
  // promise. Should there be different variants?
  Dimensions a({Dim::Y, Dim::X}, {3, 2});
  Dimensions b({Dim::X, Dim::Y}, {2, 3});
  EXPECT_EQ(merge(a, b), Dimensions({Dim::Y, Dim::X}, {3, 2}));
}

TEST(DimensionsTest, merge_size_fail) {
  Dimensions a(Dim::X, 2);
  Dimensions b({Dim::Y, Dim::X}, {3, 4});
  EXPECT_THROW(merge(a, b), except::DimensionError);
}

TEST(DimensionsTest, index) {
  Dimensions dims({Dim::X, Dim::Y}, {1, 2});
  ASSERT_THROW(dims.index(Dim::Invalid), except::DimensionError);
  ASSERT_THROW(dims.index(Dim::Z), except::DimensionNotFoundError);
  EXPECT_EQ(dims.index(Dim::X), 0);
  EXPECT_EQ(dims.index(Dim::Y), 1);
}

TEST(DimensionsTest, transpose_0d) {
  Dimensions dims;
  EXPECT_EQ(transpose(dims), dims);
  EXPECT_THROW(transpose(dims, {Dim::X}), except::DimensionError);
}

TEST(DimensionsTest, transpose_1d) {
  Dimensions dims(Dim::X, 2);
  EXPECT_EQ(transpose(dims), dims);
  EXPECT_EQ(transpose(dims, {Dim::X}), dims);
  EXPECT_THROW(transpose(dims, {Dim::Y}), except::DimensionError);
  EXPECT_THROW(transpose(dims, {Dim::X, Dim::Y}), except::DimensionError);
}

TEST(DimensionsTest, transpose_2d) {
  Dimensions dims({Dim::X, Dim::Y}, {2, 3});
  Dimensions expected({Dim::Y, Dim::X}, {3, 2});
  EXPECT_EQ(transpose(dims), expected);
  EXPECT_EQ(transpose(dims, {Dim::X, Dim::Y}), dims); // no change
  EXPECT_EQ(transpose(dims, {Dim::Y, Dim::X}), expected);
  EXPECT_THROW(transpose(dims, {Dim::X}), except::DimensionError);
  EXPECT_THROW(transpose(dims, {Dim::X, Dim::Z}), except::DimensionError);
  EXPECT_THROW(transpose(dims, {Dim::X, Dim::Y, Dim::Z}),
               except::DimensionError);
}

TEST(DimensionsTest, transpose_3d) {
  Dimensions xyz({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});
  Dimensions zyx({Dim::Z, Dim::Y, Dim::X}, {4, 3, 2});
  Dimensions zxy({Dim::Z, Dim::X, Dim::Y}, {4, 2, 3});
  EXPECT_EQ(transpose(xyz), zyx);
  EXPECT_EQ(transpose(xyz, {Dim::X, Dim::Y, Dim::Z}), xyz); // no change
  EXPECT_EQ(transpose(xyz, {Dim::Z, Dim::Y, Dim::X}), zyx);
  EXPECT_EQ(transpose(xyz, {Dim::Z, Dim::X, Dim::Y}), zxy);
  EXPECT_THROW(transpose(xyz, {Dim::X, Dim::Z}), except::DimensionError);
}
