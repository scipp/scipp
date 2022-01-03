// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <limits>

#include "scipp/core/element/to_unit.h"

using namespace scipp;
using namespace scipp::core;
using scipp::core::element::to_unit;

TEST(ElementToUnitTest, unit) {
  // Unit is simply replaced, not multiplied
  EXPECT_EQ(to_unit(units::s, units::us), units::us);
}

TEST(ElementToUnitTest, type_preserved) {
  EXPECT_TRUE((std::is_same_v<decltype(to_unit(double(1), 1)), double>));
  EXPECT_TRUE((std::is_same_v<decltype(to_unit(float(1), 1)), float>));
  EXPECT_TRUE((std::is_same_v<decltype(to_unit(int64_t(1), 1)), int64_t>));
  EXPECT_TRUE((std::is_same_v<decltype(to_unit(int32_t(1), 1)), int32_t>));
  EXPECT_TRUE((std::is_same_v<decltype(to_unit(core::time_point(1), 1)),
                              core::time_point>));
}

TEST(ElementToUnitTest, double) {
  EXPECT_EQ(to_unit(0.123456, 0.1), 0.123456 * 0.1);
}

TEST(ElementToUnitTest, float) {
  EXPECT_EQ(to_unit(0.123f, 0.1), 0.123f * 0.1f);
}

TEST(ElementToUnitTest, int64) {
  EXPECT_EQ(to_unit(int64_t(1), 0.1), 0);
  EXPECT_EQ(to_unit(int64_t(5), 0.1), 1); // 0.5 rounds up
  EXPECT_EQ(to_unit(int64_t(1), 1e6), 1000000);
  EXPECT_EQ(to_unit(int64_t(1), 1e10), 10000000000);
  EXPECT_EQ(to_unit(int64_t(13140985739), 1), 13140985739);
}

TEST(ElementToUnitTest, int32) {
  EXPECT_EQ(to_unit(int32_t(-100), 0.1), -10);
  EXPECT_EQ(to_unit(int32_t(-11), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-10), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-9), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-5), 0.1), -1);
  EXPECT_EQ(to_unit(int32_t(-4), 0.1), 0);
  EXPECT_EQ(to_unit(int32_t(1), 0.1), 0);
  EXPECT_EQ(to_unit(int32_t(5), 0.1), 1); // 0.5 rounds up
  EXPECT_EQ(to_unit(int32_t(1), 1e6), 1000000);
  EXPECT_EQ(to_unit(std::numeric_limits<int32_t>::max(), 1),
            std::numeric_limits<int32_t>::max());
}

TEST(ElementToUnitTest, int_range_exceeded) {
  // We could consider using something like boost::numeric_cast to avoid this,
  // but it is not clear whether throwing exceptions on a per-element basis
  // would be desirable?
  EXPECT_EQ(to_unit(int32_t(1), 1e10), std::numeric_limits<int32_t>::max());
  EXPECT_EQ(to_unit(int32_t(-1), 1e10), std::numeric_limits<int32_t>::min());
  EXPECT_EQ(to_unit(int64_t(1), 1e20), std::numeric_limits<int64_t>::max());
  EXPECT_EQ(to_unit(int64_t(-1), 1e20), std::numeric_limits<int64_t>::min());
}

TEST(ElementToUnitTest, time_point) {
  EXPECT_EQ(to_unit(time_point(0), 0.1), time_point(0));
  EXPECT_EQ(to_unit(time_point(5), 0.1), time_point(1)); // 0.5 rounds up
  EXPECT_EQ(to_unit(time_point(1), 1e6), time_point(1000000));
}
