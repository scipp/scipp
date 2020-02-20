// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
  }

protected:
  Dimensions xy;
  Dimensions xy_x_edges;
  Dimensions yx;
  Dimensions x;
  Dimensions y;
  Dimensions none;
};

TEST_F(ViewIndex2DTest, construct) {
  EXPECT_NO_THROW(ViewIndex(xy, none));
  EXPECT_NO_THROW(ViewIndex(xy, xy));
  EXPECT_NO_THROW(ViewIndex(xy, yx));
}

TEST_F(ViewIndex2DTest, setIndex_2D) {
  ViewIndex i(xy, xy);
  EXPECT_EQ(i.get(), 0);
  i.setIndex(1);
  EXPECT_EQ(i.get(), 1);
  i.setIndex(3);
  EXPECT_EQ(i.get(), 3);
}

TEST_F(ViewIndex2DTest, setIndex_2D_transpose) {
  ViewIndex i(xy, yx);
  EXPECT_EQ(i.get(), 0);
  i.setIndex(1);
  EXPECT_EQ(i.get(), 5);
  i.setIndex(3);
  EXPECT_EQ(i.get(), 1);
}

TEST_F(ViewIndex2DTest, increment_2D) {
  ViewIndex i(xy, xy);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);
  i.increment();
  EXPECT_EQ(i.get(), 3);
}

TEST_F(ViewIndex2DTest, end) {
  ViewIndex it(xy, xy);
  ViewIndex end(xy, xy);
  end.setIndex(3 * 5);
  for (scipp::index i = 0; i < 3 * 5; ++i) {
    EXPECT_FALSE(it == end);
    it.increment();
  }
  EXPECT_TRUE(it == end);
}

TEST_F(ViewIndex2DTest, equal) {
  ViewIndex i(xy, xy);
  ViewIndex j(xy, xy);
  i.setIndex(3 * 3);
  j.setIndex(3 * 3);
  EXPECT_TRUE(i == j);
  i.increment();
  EXPECT_FALSE(i == j);
  j.increment();
  EXPECT_TRUE(i == j);
}

TEST_F(ViewIndex2DTest, increment_2D_transpose) {
  ViewIndex i(xy, yx);
  std::vector<scipp::index> expected{0,  5, 10, 1,  6, 11, 2, 7,
                                     12, 3, 8,  13, 4, 9,  14};
  for (const auto correct : expected) {
    EXPECT_EQ(i.get(), correct);
    i.increment();
  }
}

TEST_F(ViewIndex2DTest, increment_1D) {
  ViewIndex i(xy, x);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);
  i.increment();
  EXPECT_EQ(i.get(), 0);
}

TEST_F(ViewIndex2DTest, increment_0D) {
  ViewIndex i(xy, none);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 0);
}

TEST_F(ViewIndex2DTest, fixed_dimensions) {
  ViewIndex i(x, xy);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 1);
  i.increment();
  EXPECT_EQ(i.get(), 2);
}

TEST_F(ViewIndex2DTest, fixed_dimensions_transposed) {
  ViewIndex i(x, yx);
  EXPECT_EQ(i.get(), 0);
  i.increment();
  EXPECT_EQ(i.get(), 5);
  i.increment();
  EXPECT_EQ(i.get(), 10);
}

TEST_F(ViewIndex2DTest, edges) {
  ViewIndex i(xy, xy_x_edges);
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
