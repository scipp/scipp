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

class ViewIndexNextTest : public ::testing::Test {
protected:
  ViewIndex
  make(const Dimensions &dims, const Dimensions &nested,
       const std::vector<std::pair<scipp::index, scipp::index>> &ranges) {
    return {dims, dims, nested, Dim::Row,
            scipp::span(ranges.data(), ranges.data() + 3)};
  }

  ViewIndex
  make(const Dimensions &target, const Dimensions &outer,
       const Dimensions &nested,
       const std::vector<std::pair<scipp::index, scipp::index>> &ranges) {
    return {target, outer, nested, Dim::Row,
            scipp::span(ranges.data(), ranges.data() + ranges.size())};
  }

  void check(ViewIndex i, const std::vector<scipp::index> &indices) {
    for (const auto index : indices) {
      EXPECT_EQ(i.get(), index);
      i.increment();
    }
  }
};

TEST_F(ViewIndexNextTest, outer_1d_inner_1d) {
  check(make({{Dim::X}, 3}, {{Dim::Row}, 4}, {{0, 3}, {3, 3}, {3, 4}}),
        {0, 1, 2, 3});
  check(make({{Dim::X}, 3}, {{Dim::Row}, 4}, {{0, 2}, {3, 3}, {3, 4}}),
        {0, 1, 3});
  check(make({{Dim::X}, 3}, {{Dim::Row}, 4}, {{0, 4}, {3, 3}, {3, 4}}),
        {0, 1, 2, 3, 3});
  check(make({{Dim::X}, 3}, {{Dim::Row}, 4}, {{1, 3}, {0, 2}, {2, 4}}),
        {1, 2, 0, 1, 2, 3});
}

TEST_F(ViewIndexNextTest, outer_2d_inner_1d) {
  check(make({{Dim::X, Dim::Y}, {2, 2}}, {{Dim::Row}, 6},
             {{0, 3}, {3, 3}, {3, 4}, {4, 6}}),
        {0, 1, 2, 3, 4, 5});
}

TEST_F(ViewIndexNextTest, outer_2d_transpose_inner_1d) {
  const Dimensions xy({{Dim::X, Dim::Y}, {2, 3}});
  const Dimensions yx({{Dim::Y, Dim::X}, {3, 2}});
  check(make(xy, yx, {{Dim::Row}, 6},
             {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 6}}),
        {0, 2, 4, 1, 3, 5});
  // same length in all subranges
  check(make(xy, yx, {{Dim::Row}, 12},
             {{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 10}, {10, 12}}),
        {0, 1, 4, 5, 8, 9, 2, 3, 6, 7, 10, 11});
  // 1 subrange is longer
  check(make(xy, yx, {{Dim::Row}, 13},
             {{0, 2}, {2, 4}, {4, 7}, {7, 9}, {9, 11}, {11, 13}}),
        {0, 1, 4, 5, 6, 9, 10, 2, 3, 7, 8, 11, 12});
}

TEST_F(ViewIndexNextTest, outer_1d_inner_2d_slow_ranges) {
  check(make({{Dim::X}, 3}, {{Dim::Row, Dim::Y}, {6, 2}},
             {{0, 1}, {1, 3}, {3, 6}}),
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  check(make({{Dim::X}, 3}, {{Dim::Row, Dim::Y}, {6, 2}},
             {{0, 1}, {0, 2}, {3, 6}}),
        {0, 1, 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12});
}

TEST_F(ViewIndexNextTest, outer_1d_inner_2d_fast_ranges) {
  check(make({{Dim::X}, 3}, {{Dim::Y, Dim::Row}, {2, 6}},
             {{0, 1}, {1, 3}, {3, 6}}),
        {0, 6, 1, 2, 7, 8, 3, 4, 5, 9, 10, 11});
  check(make({{Dim::X}, 3}, {{Dim::Y, Dim::Row}, {2, 6}},
             {{0, 1}, {0, 2}, {3, 6}}),
        {0, 6, 0, 1, 6, 7, 3, 4, 5, 9, 10, 11});
}
