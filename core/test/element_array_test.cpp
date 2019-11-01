// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <vector>

#include "scipp/core/element_array.h"

using scipp::core::detail::default_init_elements;
using scipp::core::detail::element_array;

static auto make_element_array() {
  std::vector<double> v{1.1, 2.2, 3.3};
  return element_array<float>(v.begin(), v.end());
}

template <class T> void check_element_array(const element_array<T> x) {
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 3);
  ASSERT_FALSE(x.empty());
  ASSERT_NE(x.data(), nullptr);
  ASSERT_EQ(x.data()[0], 1.1f);
  ASSERT_EQ(x.data()[1], 2.2f);
  ASSERT_EQ(x.data()[2], 3.3f);
}

template <class T> void check_null_element_array(const element_array<T> x) {
  ASSERT_FALSE(x);
  ASSERT_FALSE(x.empty());
  ASSERT_EQ(x.size(), -1);
  ASSERT_EQ(x.data(), nullptr);
}

TEST(ElementArrayTest, construct_default) {
  element_array<double> x;
  check_null_element_array(x);
}

TEST(ElementArrayTest, construct_size) {
  element_array<int64_t> x(2);
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 2);
  ASSERT_FALSE(x.empty());
  ASSERT_NE(x.data(), nullptr);
  ASSERT_EQ(x.data()[0], 0);
  ASSERT_EQ(x.data()[1], 0);
}

TEST(ElementArrayTest, construct_size_and_value) {
  element_array<int64_t> x(2, 7);
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 2);
  ASSERT_FALSE(x.empty());
  ASSERT_NE(x.data(), nullptr);
  ASSERT_EQ(x.data()[0], 7);
  ASSERT_EQ(x.data()[1], 7);
}

TEST(ElementArrayTest, construct_size_default_init) {
  element_array<int64_t> x(2, default_init_elements);
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 2);
  ASSERT_FALSE(x.empty());
  ASSERT_NE(x.data(), nullptr);
}

TEST(ElementArrayTest, construct_iterators) {
  auto x = make_element_array();
  check_element_array(x);
}

TEST(ElementArrayTest, construct_initializer_list) {
  element_array<float> x({1.1, 2.2, 3.3});
  check_element_array(x);
}

TEST(ElementArrayTest, construct_move) {
  auto x = make_element_array();
  const auto ptr = x.data();
  auto y(std::move(x));
  ASSERT_FALSE(x);
  ASSERT_FALSE(x.empty());
  ASSERT_EQ(x.size(), -1);
  ASSERT_EQ(y.data(), ptr);
  check_element_array(y);
}

TEST(ElementArrayTest, construct_copy) {
  auto x = make_element_array();
  auto y(x);
  ASSERT_TRUE(x);
  check_element_array(y);
}

TEST(ElementArrayTest, assign_move) {
  auto x = make_element_array();
  const auto ptr = x.data();
  element_array<float> y;
  y = std::move(x);
  ASSERT_FALSE(x);
  ASSERT_FALSE(x.empty());
  ASSERT_EQ(x.size(), -1);
  ASSERT_EQ(y.data(), ptr);
  check_element_array(y);
}

TEST(ElementArrayTest, assign_copy) {
  auto x = make_element_array();
  element_array<float> y;
  y = x;
  ASSERT_TRUE(x);
  check_element_array(y);
}

TEST(ElementArrayTest, reset) {
  auto x = make_element_array();
  x.reset();
  check_null_element_array(x);
}

TEST(ElementArrayTest, resize) {
  auto x = make_element_array();
  x.resize(2);
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 2);
  ASSERT_NE(x.data(), nullptr);
  ASSERT_EQ(x.data()[0], 0.0f);
  ASSERT_EQ(x.data()[1], 0.0f);
  ASSERT_FALSE(x.empty());
  x.resize(0);
  ASSERT_TRUE(x.empty());
}

TEST(ElementArrayTest, resize_no_init) {
  auto x = make_element_array();
  x.resize_no_init(2);
  ASSERT_TRUE(x);
  ASSERT_EQ(x.size(), 2);
  ASSERT_NE(x.data(), nullptr);

  // Data values could be anything, no assert.

  ASSERT_FALSE(x.empty());
  x.resize_no_init(0);
  ASSERT_TRUE(x.empty());
}
