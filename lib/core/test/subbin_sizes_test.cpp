// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/subbin_sizes.h"

using namespace scipp;
using namespace scipp::core;

class SubbinSizesTest : public ::testing::Test {};

TEST_F(SubbinSizesTest, comparison) {
  EXPECT_TRUE(SubbinSizes(2, {1, 2, 3}) == SubbinSizes(2, {1, 2, 3}));
  EXPECT_FALSE(SubbinSizes(2, {1, 2, 3}) == SubbinSizes(2, {1, 2}));
  EXPECT_FALSE(SubbinSizes(2, {1, 2, 3}) == SubbinSizes(2, {1, 2, 4}));
  EXPECT_FALSE(SubbinSizes(2, {1, 2, 3}) == SubbinSizes(3, {1, 2, 3}));
  // If this was a generic type we should probably return true here, but as it
  // is `==` is mainly used for unit tests, and we care about extra wasted space
  // from leading or trailing zeros.
  EXPECT_FALSE(SubbinSizes(2, {1, 2, 3}) == SubbinSizes(2, {1, 2, 3, 0}));
  EXPECT_FALSE(SubbinSizes(2, {1, 2, 3}) == SubbinSizes(1, {0, 1, 2, 3}));
}

TEST_F(SubbinSizesTest, assign_value) {
  SubbinSizes x(2, {1, 2, 3});
  x = 4;
  EXPECT_EQ(x, SubbinSizes(2, {4, 4, 4}));
}

TEST_F(SubbinSizesTest, plus_equals_empty) {
  SubbinSizes x(0, {});
  EXPECT_EQ(x += SubbinSizes({0, {}}), SubbinSizes(0, {}));
}

TEST_F(SubbinSizesTest, plus_equals) {
  SubbinSizes x(4, {1, 2, 3});
  EXPECT_EQ(x += SubbinSizes({5, {4, 5, 6}}), SubbinSizes(4, {1, 6, 8, 6}));
  EXPECT_EQ(x += SubbinSizes({6, {-1}}), SubbinSizes(4, {1, 6, 7, 6}));
  EXPECT_EQ(x += SubbinSizes({3, {1, 2}}), SubbinSizes(3, {1, 3, 6, 7, 6}));
  EXPECT_EQ(x += SubbinSizes({9, {1}}), SubbinSizes(3, {1, 3, 6, 7, 6, 0, 1}));
  EXPECT_EQ(x += SubbinSizes({0, {1}}),
            SubbinSizes(0, {1, 0, 0, 1, 3, 6, 7, 6, 0, 1}));
}

TEST_F(SubbinSizesTest, minus_equals) {
  SubbinSizes x(4, {1, 2, 3});
  EXPECT_EQ(x -= SubbinSizes({5, {-4, -5, -6}}), SubbinSizes(4, {1, 6, 8, 6}));
  EXPECT_EQ(x -= SubbinSizes({6, {1}}), SubbinSizes(4, {1, 6, 7, 6}));
  EXPECT_EQ(x -= SubbinSizes({3, {-1, -2}}), SubbinSizes(3, {1, 3, 6, 7, 6}));
  EXPECT_EQ(x -= SubbinSizes({9, {-1}}), SubbinSizes(3, {1, 3, 6, 7, 6, 0, 1}));
  EXPECT_EQ(x -= SubbinSizes({0, {-1}}),
            SubbinSizes(0, {1, 0, 0, 1, 3, 6, 7, 6, 0, 1}));
}

TEST_F(SubbinSizesTest, cumsum) {
  SubbinSizes x(2, {1, 2, 3});
  EXPECT_EQ(x.cumsum_exclusive(), SubbinSizes(2, {0, 1, 3}));
}

TEST_F(SubbinSizesTest, sum) {
  SubbinSizes x(2, {1, 2, 3});
  EXPECT_EQ(x.sum(), 6);
}

TEST_F(SubbinSizesTest, exclusive_scan) {
  SubbinSizes accum(1, {1, 2, 3});
  SubbinSizes x(2, {2, 0, 3});
  accum.exclusive_scan(x);
  EXPECT_EQ(accum, SubbinSizes(2, {2 + 2, 3 + 0, 0 + 3}));
  EXPECT_EQ(x, SubbinSizes(2, {2, 3, 0}));
}

TEST_F(SubbinSizesTest, exclusive_scan_empty_accum) {
  SubbinSizes accum(1, {});
  SubbinSizes x(2, {2, 0, 3});
  accum.exclusive_scan(x);
  EXPECT_EQ(accum, SubbinSizes(2, {2, 0, 3}));
  EXPECT_EQ(x, SubbinSizes(2, {0, 0, 0}));
}

TEST_F(SubbinSizesTest, exclusive_scan_empty_value) {
  SubbinSizes accum(1, {1, 2, 3});
  SubbinSizes x(2, {});
  accum.exclusive_scan(x);
  EXPECT_EQ(accum, SubbinSizes(2, {}));
  EXPECT_EQ(x, SubbinSizes(2, {}));
}

TEST_F(SubbinSizesTest, add_intersection) {
  SubbinSizes x(2, {1, 2, 3});
  // no overlap
  EXPECT_EQ(x.add_intersection({1, {1}}), SubbinSizes(2, {1, 2, 3}));
  EXPECT_EQ(x.add_intersection({5, {1}}), SubbinSizes(2, {1, 2, 3}));
  // partial overlap
  EXPECT_EQ(x.add_intersection({1, {1, 2}}), SubbinSizes(2, {3, 2, 3}));
  EXPECT_EQ(x.add_intersection({4, {1, 2}}), SubbinSizes(2, {3, 2, 4}));
  // inside
  EXPECT_EQ(x.add_intersection({3, {1}}), SubbinSizes(2, {3, 3, 4}));
  // touching lower
  EXPECT_EQ(x.add_intersection({2, {1, 2}}), SubbinSizes(2, {4, 5, 4}));
  // touching upper
  EXPECT_EQ(x.add_intersection({3, {1, 2}}), SubbinSizes(2, {4, 6, 6}));
  // exceeding both
  EXPECT_EQ(x.add_intersection({1, {1, 2, 3, 4, 5}}),
            SubbinSizes(2, {6, 9, 10}));
}

TEST_F(SubbinSizesTest, plus) {
  SubbinSizes x(2, {1, 2, 3});
  EXPECT_EQ(x + SubbinSizes({3, {4, 5, 6}}), SubbinSizes(2, {1, 6, 8, 6}));
  EXPECT_EQ(x + SubbinSizes({3, {1}}), SubbinSizes(2, {1, 3, 3}));
  EXPECT_EQ(x + SubbinSizes({1, {1, 2}}), SubbinSizes(1, {1, 3, 2, 3}));
  EXPECT_EQ(x + SubbinSizes({6, {1}}), SubbinSizes(2, {1, 2, 3, 0, 1}));
  EXPECT_EQ(x + SubbinSizes({0, {1}}), SubbinSizes(0, {1, 0, 1, 2, 3}));
}

TEST_F(SubbinSizesTest, minus) {
  SubbinSizes x(2, {1, 2, 3});
  EXPECT_EQ(x - SubbinSizes({3, {-4, -5, -6}}), SubbinSizes(2, {1, 6, 8, 6}));
  EXPECT_EQ(x - SubbinSizes({3, {-1}}), SubbinSizes(2, {1, 3, 3}));
  EXPECT_EQ(x - SubbinSizes({1, {-1, -2}}), SubbinSizes(1, {1, 3, 2, 3}));
  EXPECT_EQ(x - SubbinSizes({6, {-1}}), SubbinSizes(2, {1, 2, 3, 0, 1}));
  EXPECT_EQ(x - SubbinSizes({0, {-1}}), SubbinSizes(0, {1, 0, 1, 2, 3}));
}
