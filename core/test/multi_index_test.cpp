// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "multi_index.h"

using namespace scipp;
using namespace scipp::core;

class MultiIndex2DTest : public ::testing::Test {
public:
  MultiIndex2DTest() {
    const int xlen = 3;
    const int ylen = 5;

    xy.add(Dim::X, xlen);
    xy.add(Dim::Y, ylen);

    xy_x_edges.add(Dim::X, xlen + 1);
    xy_x_edges.add(Dim::Y, ylen);

    yx.add(Dim::Y, ylen);
    yx.add(Dim::X, xlen);

    x.add(Dim::X, xlen);

    y.add(Dim::Y, ylen);
  }

protected:
  Dimensions xy;
  Dimensions xy_x_edges;
  Dimensions yx;
  Dimensions x;
  Dimensions y;
  Dimensions none;
};

TEST_F(MultiIndex2DTest, construct) {
  EXPECT_NO_THROW(MultiIndex(xy, {}));
  EXPECT_NO_THROW(MultiIndex(xy, {xy}));
  EXPECT_NO_THROW(MultiIndex(xy, {yx}));
  EXPECT_NO_THROW(MultiIndex(xy, {xy, yx, x, none}));
}

TEST_F(MultiIndex2DTest, construct_fail) {
  EXPECT_THROW_MSG(MultiIndex(xy, {x, x, x, x, x}), std::runtime_error,
                   "MultiIndex supports at most 4 subindices.");
}

TEST_F(MultiIndex2DTest, setIndex_2D) {
  MultiIndex i(xy, {xy});
  EXPECT_EQ(i.get<0>(), 0);
  i.setIndex(1);
  EXPECT_EQ(i.get<0>(), 1);
  i.setIndex(3);
  EXPECT_EQ(i.get<0>(), 3);
}

TEST_F(MultiIndex2DTest, setIndex_2D_transpose) {
  MultiIndex i(xy, {yx});
  EXPECT_EQ(i.get<0>(), 0);
  i.setIndex(1);
  EXPECT_EQ(i.get<0>(), 5);
  i.setIndex(3);
  EXPECT_EQ(i.get<0>(), 1);
}

TEST_F(MultiIndex2DTest, increment_2D) {
  MultiIndex i(xy, {xy});
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 3);
}

TEST_F(MultiIndex2DTest, end) {
  MultiIndex it(xy, {xy});
  MultiIndex end(xy, {xy});
  end.setIndex(3 * 5);
  for (scipp::index i = 0; i < 3 * 5; ++i) {
    EXPECT_FALSE(it == end);
    it.increment();
  }
  EXPECT_TRUE(it == end);
}

TEST_F(MultiIndex2DTest, increment_2D_transpose) {
  MultiIndex i(xy, {yx});
  std::vector<scipp::index> expected{0,  5, 10, 1,  6, 11, 2, 7,
                                     12, 3, 8,  13, 4, 9,  14};
  for (const auto correct : expected) {
    EXPECT_EQ(i.get<0>(), correct);
    i.increment();
  }
}

TEST_F(MultiIndex2DTest, increment_1D) {
  MultiIndex i(xy, {x});
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 0);
}

TEST_F(MultiIndex2DTest, increment_0D) {
  MultiIndex i(xy, {none});
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 0);
}

TEST_F(MultiIndex2DTest, fixed_dimensions) {
  MultiIndex i(x, {xy});
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 2);
}

TEST_F(MultiIndex2DTest, fixed_dimensions_transposed) {
  MultiIndex i(x, {yx});
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 5);
  i.increment();
  EXPECT_EQ(i.get<0>(), 10);
}

TEST_F(MultiIndex2DTest, edges) {
  MultiIndex i(xy, {xy_x_edges});
  EXPECT_EQ(i.get<0>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 2);

  i.increment();
  EXPECT_EQ(i.get<0>(), 4);
  i.increment();
  EXPECT_EQ(i.get<0>(), 5);
  i.increment();
  EXPECT_EQ(i.get<0>(), 6);

  i.increment();
  EXPECT_EQ(i.get<0>(), 8);
  i.increment();
  EXPECT_EQ(i.get<0>(), 9);
  i.increment();
  EXPECT_EQ(i.get<0>(), 10);

  i.increment();
  EXPECT_EQ(i.get<0>(), 12);
  i.increment();
  EXPECT_EQ(i.get<0>(), 13);
  i.increment();
  EXPECT_EQ(i.get<0>(), 14);

  i.increment();
  EXPECT_EQ(i.get<0>(), 16);
  i.increment();
  EXPECT_EQ(i.get<0>(), 17);
  i.increment();
  EXPECT_EQ(i.get<0>(), 18);
}

class MultiIndex3DTest : public ::testing::Test {
public:
  MultiIndex3DTest() {
    const int xlen = 3;
    const int ylen = 5;
    const int zlen = 2;

    xyz.add(Dim::X, xlen);
    xyz.add(Dim::Y, ylen);
    xyz.add(Dim::Z, zlen);

    yxz.add(Dim::Y, ylen);
    yxz.add(Dim::X, xlen);
    yxz.add(Dim::Z, zlen);

    zyx.add(Dim::Z, zlen);
    zyx.add(Dim::Y, ylen);
    zyx.add(Dim::X, xlen);

    yx.add(Dim::Y, ylen);
    yx.add(Dim::X, xlen);

    x.add(Dim::X, xlen);

    y.add(Dim::Y, ylen);
  }

protected:
  Dimensions xyz;
  Dimensions yxz;
  Dimensions zyx;
  Dimensions yx;
  Dimensions x;
  Dimensions y;
  Dimensions none;
};

TEST_F(MultiIndex3DTest, construct) {
  EXPECT_NO_THROW(MultiIndex(xyz, {}));
  EXPECT_NO_THROW(MultiIndex(xyz, {xyz}));
  EXPECT_NO_THROW(MultiIndex(xyz, {zyx}));
  EXPECT_NO_THROW(MultiIndex(xyz, {xyz, yxz, yx, none}));
}

TEST_F(MultiIndex3DTest, increment_0D) {
  MultiIndex i(xyz, {none});
  for (scipp::index n = 0; n < 2 * 3 * 5; ++n) {
    EXPECT_EQ(i.get<0>(), 0);
    i.increment();
  }
}

TEST_F(MultiIndex3DTest, increment_3D) {
  MultiIndex i(xyz, {xyz, yxz, zyx});
  // y=0, z=0
  EXPECT_EQ(i.get<0>(), 0);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 1);
  EXPECT_EQ(i.get<1>(), 5);
  EXPECT_EQ(i.get<2>(), 10);
  i.increment();
  EXPECT_EQ(i.get<0>(), 2);
  EXPECT_EQ(i.get<1>(), 10);
  EXPECT_EQ(i.get<2>(), 20);
  i.increment();

  // y=1, z=0
  EXPECT_EQ(i.get<0>(), 3);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 4);
  EXPECT_EQ(i.get<1>(), 6);
  EXPECT_EQ(i.get<2>(), 12);
  i.increment();
  EXPECT_EQ(i.get<0>(), 5);
  EXPECT_EQ(i.get<1>(), 11);
  EXPECT_EQ(i.get<2>(), 22);

  // y=2, z=0
  i.increment();
  EXPECT_EQ(i.get<0>(), 6);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 4);
  i.increment();
  EXPECT_EQ(i.get<0>(), 7);
  EXPECT_EQ(i.get<1>(), 7);
  EXPECT_EQ(i.get<2>(), 14);
  i.increment();
  EXPECT_EQ(i.get<0>(), 8);
  EXPECT_EQ(i.get<1>(), 12);
  EXPECT_EQ(i.get<2>(), 24);

  // y=0, z=1
  i.setIndex(3 * 5);
  EXPECT_EQ(i.get<0>(), 15);
  EXPECT_EQ(i.get<1>(), 15);
  EXPECT_EQ(i.get<2>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 16);
  EXPECT_EQ(i.get<1>(), 20);
  EXPECT_EQ(i.get<2>(), 11);
  i.increment();
  EXPECT_EQ(i.get<0>(), 17);
  EXPECT_EQ(i.get<1>(), 25);
  EXPECT_EQ(i.get<2>(), 21);

  // y=1, z=1
  i.increment();
  EXPECT_EQ(i.get<0>(), 18);
  EXPECT_EQ(i.get<1>(), 16);
  EXPECT_EQ(i.get<2>(), 3);
  i.increment();
  EXPECT_EQ(i.get<0>(), 19);
  EXPECT_EQ(i.get<1>(), 21);
  EXPECT_EQ(i.get<2>(), 13);
  i.increment();
  EXPECT_EQ(i.get<0>(), 20);
  EXPECT_EQ(i.get<1>(), 26);
  EXPECT_EQ(i.get<2>(), 23);

  // y=2, z=1
  i.increment();
  EXPECT_EQ(i.get<0>(), 21);
  EXPECT_EQ(i.get<1>(), 17);
  EXPECT_EQ(i.get<2>(), 5);
  i.increment();
  EXPECT_EQ(i.get<0>(), 22);
  EXPECT_EQ(i.get<1>(), 22);
  EXPECT_EQ(i.get<2>(), 15);
  i.increment();
  EXPECT_EQ(i.get<0>(), 23);
  EXPECT_EQ(i.get<1>(), 27);
  EXPECT_EQ(i.get<2>(), 25);
}

TEST_F(MultiIndex3DTest, increment_3D_1D_1D) {
  MultiIndex i(xyz, {xyz, x, y});
  // y=0, z=0
  EXPECT_EQ(i.get<0>(), 0);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 1);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 2);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 0);
  i.increment();

  // y=1, z=0
  EXPECT_EQ(i.get<0>(), 3);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 4);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 5);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 1);

  // y=2, z=0
  i.increment();
  EXPECT_EQ(i.get<0>(), 6);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 7);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 8);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 2);

  // y=0, z=1
  i.setIndex(3 * 5);
  EXPECT_EQ(i.get<0>(), 15);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 16);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 0);
  i.increment();
  EXPECT_EQ(i.get<0>(), 17);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 0);

  // y=1, z=1
  i.increment();
  EXPECT_EQ(i.get<0>(), 18);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 19);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 1);
  i.increment();
  EXPECT_EQ(i.get<0>(), 20);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 1);

  // y=2, z=1
  i.increment();
  EXPECT_EQ(i.get<0>(), 21);
  EXPECT_EQ(i.get<1>(), 0);
  EXPECT_EQ(i.get<2>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 22);
  EXPECT_EQ(i.get<1>(), 1);
  EXPECT_EQ(i.get<2>(), 2);
  i.increment();
  EXPECT_EQ(i.get<0>(), 23);
  EXPECT_EQ(i.get<1>(), 2);
  EXPECT_EQ(i.get<2>(), 2);
}
