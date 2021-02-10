// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/time_point.h"
#include "scipp/units/except.h"

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
