// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/subbin_sizes.h"

using namespace scipp;
using namespace scipp::core;

class SubbinSizesTest : public ::testing::Test {};

TEST_F(SubbinSizesTest, trim_to) {
  SubbinSizes x(2, {1, 2, 3});
  x.trim_to({2, {6, 6, 6}});
  EXPECT_EQ(x, SubbinSizes(2, {1, 2, 3}));
  x.trim_to({3, {6, 6, 6}});
  EXPECT_EQ(x, SubbinSizes(3, {2, 3, 0}));
  x.trim_to({1, {6, 6, 6, 6, 6}});
  EXPECT_EQ(x, SubbinSizes(1, {0, 0, 2, 3, 0}));
  x.trim_to({2, {6, 6, 6}});
  EXPECT_EQ(x, SubbinSizes(2, {0, 2, 3}));
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
