// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "scipp/common/numeric.h"

using scipp::numeric::isarange;

TEST(IsArangeTest, empty) { ASSERT_TRUE(isarange(std::vector<int32_t>({}))); }

TEST(IsArangeTest, size_1) { ASSERT_TRUE(isarange(std::vector<int32_t>({1}))); }

TEST(IsArangeTest, negative) {
  ASSERT_FALSE(isarange(std::vector<int32_t>({1, 0})));
}

TEST(IsArangeTest, constant) {
  ASSERT_FALSE(isarange(std::vector<int32_t>({1, 1, 1})));
}

TEST(IsArangeTest, constant_section) {
  ASSERT_FALSE(isarange(std::vector<int32_t>({1, 1, 2})));
}

TEST(IsArangeTest, decreasing_section) {
  ASSERT_FALSE(isarange(std::vector<int32_t>({3, 2, 4})));
}

TEST(IsArangeTest, size_2) {
  ASSERT_TRUE(isarange(std::vector<int32_t>({1, 2})));
}

TEST(IsArangeTest, size_3) {
  ASSERT_TRUE(isarange(std::vector<int32_t>({1, 2, 3})));
}

TEST(IsArangeTest, negative_front) {
  ASSERT_TRUE(isarange(std::vector<int32_t>({-3, -2, -1, 0, 1, 2})));
}

TEST(IsArangeTest, std_iota) {
  std::vector<int32_t> range(1e5);
  std::iota(range.begin(), range.end(), 1);
  ASSERT_TRUE(isarange(range));
}

TEST(IsArangeTest, generate_addition) {
  std::vector<int32_t> range;
  int32_t current = 345;
  std::generate_n(std::back_inserter(range), 2ul << 16ul, [&current]() {
    const int32_t step = 1;
    current += step;
    return current;
  });

  ASSERT_TRUE(isarange(range));
}
