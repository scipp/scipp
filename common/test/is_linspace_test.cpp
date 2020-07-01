// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "scipp/common/numeric.h"

using scipp::numeric::is_linspace;

TEST(IsLinspaceTest, empty) {
  ASSERT_FALSE(is_linspace(std::vector<double>({})));
  ASSERT_FALSE(is_linspace(std::vector<int32_t>({})));
}

TEST(IsLinspaceTest, size_1) {
  ASSERT_FALSE(is_linspace(std::vector<double>({1.0})));
  ASSERT_FALSE(is_linspace(std::vector<int32_t>({1})));
}

TEST(IsLinspaceTest, negative) {
  ASSERT_FALSE(is_linspace(std::vector<double>({1.0, 0.5})));
  ASSERT_FALSE(is_linspace(std::vector<int32_t>({1, 0})));
}

TEST(IsLinspaceTest, constant) {
  ASSERT_FALSE(is_linspace(std::vector<double>({1.0, 1.0, 1.0})));
  ASSERT_FALSE(is_linspace(std::vector<int32_t>({1, 1, 1})));
}

TEST(IsLinspaceTest, constant_section) {
  ASSERT_FALSE(is_linspace(std::vector<double>({1.0, 1.0, 2.0})));
  ASSERT_FALSE(is_linspace(std::vector<int32_t>({1, 1, 2})));
}

TEST(IsLinspaceTest, decreasing_section) {
  ASSERT_FALSE(is_linspace(std::vector<double>({1.5, 1.0, 2.0})));
  ASSERT_FALSE(is_linspace(std::vector<int32_t>({3, 2, 4})));
}

TEST(IsLinspaceTest, size_2) {
  ASSERT_TRUE(is_linspace(std::vector<double>({1.0, 2.0})));
  ASSERT_TRUE(is_linspace(std::vector<int32_t>({1, 2})));
}

TEST(IsLinspaceTest, size_3) {
  ASSERT_TRUE(is_linspace(std::vector<double>({1.0, 2.0, 3.0})));
  ASSERT_TRUE(is_linspace(std::vector<int32_t>({1, 2, 3})));
}

TEST(IsLinspaceTest, negative_front) {
  ASSERT_TRUE(
      is_linspace(std::vector<double>({-3.0, -2.0, -1.0, 0.0, 1.0, 2.0})));
}

TEST(IsLinspaceTest, std_iota) {
  std::vector<double> range(1e5);
  std::iota(range.begin(), range.end(), 1e-9);
  ASSERT_TRUE(is_linspace(range));
}

TEST(IsLinspaceTest, generate_addition) {
  std::vector<double> range;
  double current = 345.4564675;
  std::generate_n(std::back_inserter(range), 1e5, [&current]() {
    const double step = 0.0034674;
    current += step;
    return current;
  });

  ASSERT_TRUE(is_linspace(range));
}
