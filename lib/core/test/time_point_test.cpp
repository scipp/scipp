// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <type_traits>

#include "scipp/core/time_point.h"

using namespace scipp;
using namespace scipp::core;

class TimePointTest : public ::testing::Test {
protected:
  const int64_t i1 = 1;
  const int32_t i2 = 2;

  const time_point t0 = time_point();
  const time_point t1 = time_point{i1};
  const time_point t2 = time_point{i2};
};

TEST_F(TimePointTest, time_since_epoch) {
  EXPECT_EQ(t0.time_since_epoch(), 0);
  EXPECT_EQ(t2.time_since_epoch(), i2);
}

TEST_F(TimePointTest, plus_minus_arithmetics) {
  EXPECT_EQ(t1 - 1, time_point{i1 - 1});
  EXPECT_EQ(t2 - t1, i2 - i1);
  EXPECT_EQ(t1 + 1, t2);
  EXPECT_EQ(1 + t1, t2);
}

TEST_F(TimePointTest, inequalities_arithmetics) {
  EXPECT_TRUE(t1 < t2);
  EXPECT_TRUE(t2 > t1);
  EXPECT_TRUE(t1 <= t2);
  EXPECT_TRUE(t2 >= t1);
  EXPECT_TRUE(t1 != t2);
  EXPECT_TRUE(t1 == t1);
}

TEST_F(TimePointTest, inplace_arithmetics) {
  time_point aux = t2;
  EXPECT_EQ(aux -= 1, t1);
  EXPECT_EQ(aux += 1, t2);
}

TEST(TimePointTypeTest, is_pod) {
  // It is essential that time_point is a POD-type, to ensure that operations
  // such as sc.empty are fast and do not call constructors and initialize
  // memory --- in particular since this is also used internally for
  // initialization the output in the implementation of many operations.
  ASSERT_TRUE(std::is_trivial_v<time_point>);
  ASSERT_TRUE(std::is_standard_layout_v<time_point>);
}
