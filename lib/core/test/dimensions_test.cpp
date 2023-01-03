// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <type_traits>

#include "test_macros.h"

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"

using namespace scipp;
using namespace scipp::core;

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
  dims.add(Dim::X, 3);
  EXPECT_EQ(dims.shape().size(), 1);
  EXPECT_EQ(dims.volume(), 3);
  dims.add(Dim::Y, 2);
  EXPECT_EQ(dims.shape().size(), 2);
  EXPECT_EQ(dims.volume(), 6);
}

TEST(DimensionsTest, offset_from_list_init) {
  // Leftmost is outer dimension, rightmost is inner dimension.
  Dimensions dims{{Dim::Y, 2}, {Dim::X, 3}};
  EXPECT_EQ(dims.offset(Dim::X), 1);
  EXPECT_EQ(dims.offset(Dim::Y), 3);
}

TEST(DimensionsTest, offset) {
  Dimensions dims;
  dims.add(Dim::X, 3);
  dims.add(Dim::Y, 2);
  EXPECT_EQ(dims.offset(Dim::X), 1);
  EXPECT_EQ(dims.offset(Dim::Y), 3);
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

TEST(DimensionsTest, includes) {
  Dimensions a;
  a.add(Dim::X, 3);
  a.add(Dim::Y, 2);

  EXPECT_TRUE(a.includes(Dimensions{}));
  EXPECT_TRUE(a.includes(a));
  EXPECT_TRUE(a.includes(Dimensions(Dim::Y, 2)));
  EXPECT_FALSE(a.includes(Dimensions(Dim::Y, 3)));

  Dimensions b;
  b.add(Dim::Y, 2);
  b.add(Dim::X, 3);
  // Order does not matter.
  EXPECT_TRUE(a.includes(b));
}

TEST(DimensionsTest, index_access) {
  Dimensions denseXY({Dim::X, Dim::Y}, {2, 3});
  Dimensions denseXYZ({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});

  ASSERT_THROW_DISCARD(denseXY[Dim::Invalid], except::DimensionError);
  ASSERT_THROW_DISCARD(denseXYZ[Dim::Invalid], except::DimensionError);
  ASSERT_THROW_DISCARD(denseXY[Dim::Z], except::DimensionError);
  ASSERT_NO_THROW_DISCARD(denseXYZ[Dim::Z]);
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

TEST(DimensionsTest, merge_non_overlapping) {
  Dimensions a(Dim::X, 2);
  Dimensions b({Dim::Y, Dim::Z}, {3, 4});
  EXPECT_EQ(merge(a, b), Dimensions({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}));
}

TEST(DimensionsTest, merge_overlapping) {
  Dimensions a({Dim::X, Dim::Y}, {2, 3});
  Dimensions b({Dim::Y, Dim::Z}, {3, 4});
  EXPECT_EQ(merge(a, b), Dimensions({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}));
}

TEST(DimensionsTest, merge_different_order) {
  // The implementation "favors" the order of the first argument if both
  // inputs have the same number of dimension. Transposing is avoided where
  // possible, which is crucial for accumulate performance.
  Dimensions x(Dim::X, 2);
  Dimensions yx({Dim::Y, Dim::X}, {3, 2});
  Dimensions xy({Dim::X, Dim::Y}, {2, 3});
  Dimensions xz({Dim::X, Dim::Z}, {2, 1});
  Dimensions xyz({Dim::X, Dim::Y, Dim::Z}, {2, 3, 1});
  Dimensions xzy({Dim::X, Dim::Z, Dim::Y}, {2, 1, 3});
  Dimensions zxy({Dim::Z, Dim::X, Dim::Y}, {1, 2, 3});
  EXPECT_EQ(merge(x, xy), xy);
  EXPECT_EQ(merge(x, yx), yx); // no transpose
  EXPECT_EQ(merge(yx, xy), yx);
  EXPECT_EQ(merge(xy, xyz), xyz);
  EXPECT_EQ(merge(xy, xzy), xzy); // no y-z transpose
  EXPECT_EQ(merge(xy, zxy), zxy);
  EXPECT_EQ(merge(xz, xyz), xyz);
}

TEST(DimensionsTest, merge_size_fail) {
  Dimensions a(Dim::X, 2);
  Dimensions b({Dim::Y, Dim::X}, {3, 4});
  EXPECT_THROW_DISCARD(merge(a, b), except::DimensionError);
}

TEST(DimensionsTest, intersection) {
  Dimensions x(Dim::X, 2);
  Dimensions y(Dim::Y, 3);
  Dimensions yx({Dim::Y, Dim::X}, {3, 2});
  Dimensions xy({Dim::X, Dim::Y}, {2, 3});
  Dimensions yz({Dim::Y, Dim::Z}, {3, 1});
  Dimensions xyz({Dim::X, Dim::Y, Dim::Z}, {2, 3, 1});
  Dimensions xzy({Dim::X, Dim::Z, Dim::Y}, {2, 1, 3});
  Dimensions zxy({Dim::Z, Dim::X, Dim::Y}, {1, 2, 3});
  EXPECT_EQ(intersection(x, xy), x);
  EXPECT_EQ(intersection(yx, xy), yx);
  EXPECT_EQ(intersection(xy, xyz), xy);
  EXPECT_EQ(intersection(yz, xyz), yz);
  EXPECT_EQ(intersection(yx, xzy), yx);
  EXPECT_EQ(intersection(yz, zxy), yz);
  EXPECT_EQ(intersection(x, y), Dimensions{});
}

TEST(DimensionsTest, index) {
  Dimensions dims({Dim::X, Dim::Y}, {1, 2});
  ASSERT_THROW_DISCARD(dims.index(Dim::Invalid), except::DimensionError);
  ASSERT_THROW_DISCARD(dims.index(Dim::Z), except::DimensionError);
  EXPECT_EQ(dims.index(Dim::X), 0);
  EXPECT_EQ(dims.index(Dim::Y), 1);
}

TEST(DimensionsTest, transpose_0d) {
  Dimensions dims;
  EXPECT_EQ(transpose(dims), dims);
  EXPECT_THROW_DISCARD(transpose(dims, std::vector<Dim>{Dim::X}),
                       except::DimensionError);
}

TEST(DimensionsTest, transpose_1d) {
  Dimensions dims(Dim::X, 2);
  EXPECT_EQ(transpose(dims), dims);
  EXPECT_EQ(transpose(dims, std::vector<Dim>{Dim::X}), dims);
  EXPECT_THROW_DISCARD(transpose(dims, std::vector<Dim>{Dim::Y}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(dims, std::vector<Dim>{Dim::X, Dim::Y}),
                       except::DimensionError);
}

TEST(DimensionsTest, transpose_2d) {
  Dimensions dims({Dim::X, Dim::Y}, {2, 3});
  Dimensions expected({Dim::Y, Dim::X}, {3, 2});
  EXPECT_EQ(transpose(dims), expected);
  EXPECT_EQ(transpose(dims, std::vector<Dim>{Dim::X, Dim::Y}),
            dims); // no change
  EXPECT_EQ(transpose(dims, std::vector<Dim>{Dim::Y, Dim::X}), expected);
  EXPECT_THROW_DISCARD(transpose(dims, std::vector<Dim>{Dim::X}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(transpose(dims, std::vector<Dim>{Dim::X, Dim::Z}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(
      transpose(dims, std::vector<Dim>{Dim::X, Dim::Y, Dim::Z}),
      except::DimensionError);
}

TEST(DimensionsTest, transpose_3d) {
  Dimensions xyz({Dim::X, Dim::Y, Dim::Z}, {2, 3, 4});
  Dimensions zyx({Dim::Z, Dim::Y, Dim::X}, {4, 3, 2});
  Dimensions zxy({Dim::Z, Dim::X, Dim::Y}, {4, 2, 3});
  EXPECT_EQ(transpose(xyz), zyx);
  EXPECT_EQ(transpose(xyz, std::vector<Dim>{Dim::X, Dim::Y, Dim::Z}),
            xyz); // no change
  EXPECT_EQ(transpose(xyz, std::vector<Dim>{Dim::Z, Dim::Y, Dim::X}), zyx);
  EXPECT_EQ(transpose(xyz, std::vector<Dim>{Dim::Z, Dim::X, Dim::Y}), zxy);
  EXPECT_THROW_DISCARD(transpose(xyz, std::vector<Dim>{Dim::X, Dim::Z}),
                       except::DimensionError);
}

TEST(DimensionsTest, fold) {
  Dimensions xy = {{Dim::X, 6}, {Dim::Y, 4}};
  Dimensions expected = {{Dim::Row, 2}, {Dim::Time, 3}, {Dim::Y, 4}};
  EXPECT_EQ(fold(xy, Dim::X, {{Dim::Row, 2}, {Dim::Time, 3}}), expected);
  EXPECT_THROW_DISCARD(fold(xy, Dim::Z, {{Dim::Row, 2}, {Dim::Time, 3}}),
                       except::NotFoundError);
}

TEST(DimensionsTest, fold_fail_bad_sizes) {
  Dimensions x(Dim::X, 6);
  EXPECT_NO_THROW_DISCARD(fold(x, Dim::X, {{Dim::Y, 2}, {Dim::Z, 3}}));
  EXPECT_THROW_DISCARD(fold(x, Dim::X, {{Dim::Y, 2}, {Dim::Z, 2}}),
                       except::DimensionError);
  EXPECT_THROW_DISCARD(fold(x, Dim::X, {{Dim::Y, 3}, {Dim::Z, 3}}),
                       except::DimensionError);
}

TEST(DimensionsTest, fold_into_3) {
  Dimensions x = {{Dim::X, 24}};
  Dimensions expected = {{Dim::X, 2}, {Dim::Y, 3}, {Dim::Z, 4}};
  EXPECT_EQ(fold(x, Dim::X, expected), expected);
}
