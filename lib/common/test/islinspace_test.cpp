// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "scipp/common/numeric.h"

using scipp::numeric::islinspace;

TEST(IsLinspaceTest, empty) {
  ASSERT_FALSE(islinspace(std::vector<double>({})));
  ASSERT_FALSE(islinspace(std::vector<int32_t>({})));
}

TEST(IsLinspaceTest, size_1) {
  ASSERT_FALSE(islinspace(std::vector<double>({1.0})));
  ASSERT_FALSE(islinspace(std::vector<int32_t>({1})));
}

TEST(IsLinspaceTest, negative) {
  ASSERT_FALSE(islinspace(std::vector<double>({1.0, 0.5})));
  ASSERT_FALSE(islinspace(std::vector<int32_t>({1, 0})));
}

TEST(IsLinspaceTest, constant) {
  ASSERT_FALSE(islinspace(std::vector<double>({1.0, 1.0, 1.0})));
  ASSERT_FALSE(islinspace(std::vector<int32_t>({1, 1, 1})));
}

TEST(IsLinspaceTest, constant_section) {
  ASSERT_FALSE(islinspace(std::vector<double>({1.0, 1.0, 2.0})));
  ASSERT_FALSE(islinspace(std::vector<int32_t>({1, 1, 2})));
}

TEST(IsLinspaceTest, decreasing_section) {
  ASSERT_FALSE(islinspace(std::vector<double>({1.5, 1.0, 2.0})));
  ASSERT_FALSE(islinspace(std::vector<int32_t>({3, 2, 4})));
}

TEST(IsLinspaceTest, size_2) {
  ASSERT_TRUE(islinspace(std::vector<double>({1.0, 2.0})));
  ASSERT_TRUE(islinspace(std::vector<int32_t>({1, 2})));
}

TEST(IsLinspaceTest, size_3) {
  ASSERT_TRUE(islinspace(std::vector<double>({1.0, 2.0, 3.0})));
  ASSERT_TRUE(islinspace(std::vector<int32_t>({1, 2, 3})));
}

TEST(IsLinspaceTest, negative_front) {
  ASSERT_TRUE(
      islinspace(std::vector<double>({-3.0, -2.0, -1.0, 0.0, 1.0, 2.0})));
}

TEST(IsLinspaceTest, std_iota) {
  std::vector<double> range(1e5);
  std::iota(range.begin(), range.end(), 1e-9);
  ASSERT_TRUE(islinspace(range));
}

TEST(IsLinspaceTest, generate_addition) {
  std::vector<double> range;
  double current = 345.4564675;
  std::generate_n(std::back_inserter(range), 2ul << 16ul, [&current]() {
    const double step = 0.0034674;
    current += step;
    return current;
  });

  ASSERT_TRUE(islinspace(range));
}
