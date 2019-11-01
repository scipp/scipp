// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <vector>

#include "scipp/core/element_array.h"

using scipp::core::detail::element_array;

TEST(ElementArrayTest, construct_default) {
  element_array<double> x;
  ASSERT_FALSE(x);
  ASSERT_EQ(x.size(), -1);
  ASSERT_EQ(x.data(), nullptr);
}

TEST(ElementArrayTest, construct_iterators) {
  std::vector<double> v{1.1, 2.2, 3.3};
  element_array<float> x(v.begin(), v.end());
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 3);
  ASSERT_NE(x.data(), nullptr);
  ASSERT_EQ(x.data()[0], 1.1f);
  ASSERT_EQ(x.data()[1], 2.2f);
  ASSERT_EQ(x.data()[2], 3.3f);
}
