// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/deep_ptr.h"

using scipp::deep_ptr;

struct DeepPtrTest : public ::testing::Test {
protected:
  deep_ptr<double> empty{};
  deep_ptr<double> one{std::make_unique<double>(1.0)};
  deep_ptr<double> two{std::make_unique<double>(2.0)};
};

TEST_F(DeepPtrTest, copy) {
  auto copy(one);
  EXPECT_NE(copy, one);
  EXPECT_EQ(*copy, *one);
}

TEST_F(DeepPtrTest, move) {
  const auto *ptr = one.operator->();
  auto moved(std::move(one));
  EXPECT_FALSE(one);
  EXPECT_EQ(*moved, 1.0);
  EXPECT_EQ(moved.operator->(), ptr);
}

TEST_F(DeepPtrTest, move_unique_ptr) {
  auto base = std::make_unique<double>(1.0);
  const auto *ptr = base.operator->();
  auto moved(std::move(base));
  EXPECT_FALSE(base);
  EXPECT_EQ(*moved, 1.0);
  EXPECT_EQ(moved.operator->(), ptr);
}

TEST_F(DeepPtrTest, copy_assign) {
  deep_ptr<double> copy;
  copy = one;
  EXPECT_NE(copy, one);
  EXPECT_EQ(*copy, *one);
}

TEST_F(DeepPtrTest, move_assign) {
  deep_ptr<double> moved;
  const auto *ptr = one.operator->();
  moved = std::move(one);
  EXPECT_FALSE(one);
  EXPECT_EQ(*moved, 1.0);
  EXPECT_EQ(moved.operator->(), ptr);
}

TEST_F(DeepPtrTest, operator_bool) {
  EXPECT_FALSE(empty);
  EXPECT_TRUE(one);
}

TEST_F(DeepPtrTest, compare) {
  EXPECT_TRUE(empty == empty);
  EXPECT_TRUE(one == one);
  EXPECT_FALSE(one == empty);
  EXPECT_FALSE(one == two);
  EXPECT_FALSE(empty != empty);
  EXPECT_FALSE(one != one);
  EXPECT_TRUE(one != empty);
  EXPECT_TRUE(one != two);
}

TEST_F(DeepPtrTest, dereference) {
  EXPECT_EQ(*one, 1.0);
  EXPECT_EQ(*two, 2.0);
}
