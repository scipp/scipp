// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/view_index.h"

using namespace scipp;
using namespace scipp::core;

class ViewIndex2DTest : public ::testing::Test {
public:
  ViewIndex2DTest() {
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

    s_xy_xy = Strides{xlen, 1};
    s_xy_yx = Strides{1, ylen};
    s_xy_x = Strides{0, 1};
    s_x_xy = Strides{1};
    s_x_yx = Strides{ylen};
    s_xy_xy_x_edges = Strides{xlen + 1, 1};
  }

protected:
  Dimensions xy;
  Dimensions xy_x_edges;
  Dimensions yx;
  Dimensions x;
  Dimensions y;
  Dimensions none;

  Strides s_xy_xy;
  Strides s_xy_yx;
  Strides s_xy_x;
  Strides s_x_xy;
  Strides s_x_yx;
  Strides s_xy_xy_x_edges;
};

TEST_F(ViewIndex2DTest, construct) {
  EXPECT_NO_THROW(ViewIndex(xy, s_xy_xy));
  EXPECT_NO_THROW(ViewIndex(xy, s_xy_yx));
}

TEST_F(ViewIndex2DTest, setIndex_2D) {
  ViewIndex i(xy, s_xy_xy);
  EXPECT_EQ(i.get(), 0);
  i.set_index(1);
  EXPECT_EQ(i.get(), 1);
  i.set_index(3);
  EXPECT_EQ(i.get(), 3);
}

TEST_F(ViewIndex2DTest, setIndex_2D_transpose) {
  ViewIndex i(xy, s_xy_yx);
  EXPECT_EQ(i.get(), 0);
  i.set_index(1);
  EXPECT_EQ(i.get(), 5);
  i.set_index(3);
  EXPECT_EQ(i.get(), 1);
}

TEST_F(ViewIndex2DTest, increment_2D) {
  ViewIndex i(xy, s_xy_xy);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);
  i.increment();
  EXPECT_EQ(i.get(), 3);
}

TEST_F(ViewIndex2DTest, end) {
  ViewIndex it(xy, s_xy_xy);
  ViewIndex end(xy, s_xy_xy);
  end.set_index(3 * 5);
  for (scipp::index i = 0; i < 3 * 5; ++i) {
    EXPECT_FALSE(it == end);
    it.increment();
  }
  EXPECT_TRUE(it == end);
}

TEST_F(ViewIndex2DTest, equal) {
  ViewIndex i(xy, s_xy_xy);
  ViewIndex j(xy, s_xy_xy);
  i.set_index(3 * 3);
  j.set_index(3 * 3);
  EXPECT_TRUE(i == j);
  i.increment();
  EXPECT_FALSE(i == j);
  j.increment();
  EXPECT_TRUE(i == j);
}

TEST_F(ViewIndex2DTest, increment_2D_transpose) {
  ViewIndex i(xy, s_xy_yx);
  std::vector<scipp::index> expected{0,  5, 10, 1,  6, 11, 2, 7,
                                     12, 3, 8,  13, 4, 9,  14};
  for (const auto correct : expected) {
    EXPECT_EQ(i.get(), correct);
    i.increment();
  }
}

TEST_F(ViewIndex2DTest, increment_1D) {
  ViewIndex i(xy, s_xy_x);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);
  i.increment();
  EXPECT_EQ(i.get(), 0);
}

TEST_F(ViewIndex2DTest, increment_0D) {
  ViewIndex i(xy, Strides{0, 0});
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 0);
}

TEST_F(ViewIndex2DTest, fixed_dimensions) {
  ViewIndex i(x, s_x_xy);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);
}

TEST_F(ViewIndex2DTest, fixed_dimensions_transposed) {
  ViewIndex i(x, s_x_yx);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 5);
  i.increment();
  EXPECT_EQ(i.get(), 10);
}

TEST_F(ViewIndex2DTest, edges) {
  ViewIndex i(xy, s_xy_xy_x_edges);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);

  i.increment();
  EXPECT_EQ(i.get(), 4);
  i.increment();
  EXPECT_EQ(i.get(), 5);
  i.increment();
  EXPECT_EQ(i.get(), 6);

  i.increment();
  EXPECT_EQ(i.get(), 8);
  i.increment();
  EXPECT_EQ(i.get(), 9);
  i.increment();
  EXPECT_EQ(i.get(), 10);

  i.increment();
  EXPECT_EQ(i.get(), 12);
  i.increment();
  EXPECT_EQ(i.get(), 13);
  i.increment();
  EXPECT_EQ(i.get(), 14);

  i.increment();
  EXPECT_EQ(i.get(), 16);
  i.increment();
  EXPECT_EQ(i.get(), 17);
  i.increment();
  EXPECT_EQ(i.get(), 18);
}

TEST(ViewIndexTest, empty1D) {
  Dimensions dims;
  dims.add(Dim::X, 0);
  const ViewIndex idx{dims, Strides{0}};
  EXPECT_EQ(idx.get(), 0);
  EXPECT_EQ(idx.index(), 0);
}

TEST(ViewIndexTest, empty2D) {
  Dimensions dims;
  dims.add(Dim::X, 0);
  dims.add(Dim::Y, 0);
  const ViewIndex idx{dims, Strides{0, 0}};
  EXPECT_EQ(idx.get(), 0);
  EXPECT_EQ(idx.index(), 0);
}
