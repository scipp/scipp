// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/rounding.h"

using namespace scipp::core;

TEST(ElementRoundingTest, floor) {
  EXPECT_EQ(element::floor(2.5), 2);
  EXPECT_EQ(element::floor(2.7), 2);
  EXPECT_EQ(element::floor(2.3), 2);
}

TEST(ElementRoundingTest, floorDecimals) {
  EXPECT_EQ(element::floor(2.15), 2);
  EXPECT_EQ(element::floor(2.617), 2);
  EXPECT_EQ(element::floor(2.32133), 2);
}

TEST(ElementRoundingTest, ceil) {
  EXPECT_EQ(element::ceil(2.5), 3);
  EXPECT_EQ(element::ceil(2.7), 3);
  EXPECT_EQ(element::ceil(2.3), 3);
}

TEST(ElementRoundingTest, ceilDecimals) {
  EXPECT_EQ(element::ceil(2.15), 3);
  EXPECT_EQ(element::ceil(2.617), 3);
  EXPECT_EQ(element::ceil(2.32133), 3);
}

TEST(ElementRoundingTest, round) {
  EXPECT_EQ(element::round(2.01), 2);
  EXPECT_EQ(element::round(2.7), 3);
  EXPECT_EQ(element::round(2.3), 2);

  // In the middle of two integers prefer the even number (numpy does this)
  EXPECT_EQ(element::round(1.5), 2);
  EXPECT_EQ(element::round(2.5), 2);
  EXPECT_EQ(element::round(3.5), 4);
  EXPECT_EQ(element::round(4.5), 4);
}

TEST(ElementRoundingTest, roundDecimals) {
  EXPECT_EQ(element::round(2.15, 1), 2.2);
  EXPECT_EQ(element::round(2.617, 2), 2.62);
  EXPECT_EQ(element::round(2.32133, 4), 2.3213);
}