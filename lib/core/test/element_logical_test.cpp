// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/logical.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core::element;

template <typename T> class LogicalTest : public ::testing::Test {};

TEST_F(LogicalTest, accepts_only_bool) {
  static_assert(std::is_same_v<decltype(logical)::types, std::tuple<bool>>);
  static_assert(
      std::is_same_v<decltype(logical_inplace)::types, std::tuple<bool>>);
}

TEST_F(LogicalTest, logical_unit) {
  EXPECT_EQ(logical(units::none), units::none);
  EXPECT_THROW(logical(units::m), except::UnitError);
}

TEST_F(LogicalTest, logical_inplace_unit) {
  auto u = units::none;
  EXPECT_NO_THROW(logical_inplace(u, units::none));
  EXPECT_EQ(u, units::none);
  EXPECT_THROW(logical_inplace(u, units::m), except::UnitError);
  u = units::m;
  EXPECT_THROW(logical_inplace(u, units::none), except::UnitError);
  EXPECT_THROW(logical_inplace(u, units::m), except::UnitError);
}

TEST_F(LogicalTest, and) {
  EXPECT_EQ(logical_and(true, true), true);
  EXPECT_EQ(logical_and(true, false), false);
  EXPECT_EQ(logical_and(false, true), false);
  EXPECT_EQ(logical_and(false, false), false);
}

TEST_F(LogicalTest, or) {
  EXPECT_EQ(logical_or(true, true), true);
  EXPECT_EQ(logical_or(true, false), true);
  EXPECT_EQ(logical_or(false, true), true);
  EXPECT_EQ(logical_or(false, false), false);
}

TEST_F(LogicalTest, xor) {
  EXPECT_EQ(logical_xor(true, true), false);
  EXPECT_EQ(logical_xor(true, false), true);
  EXPECT_EQ(logical_xor(false, true), true);
  EXPECT_EQ(logical_xor(false, false), false);
}

TEST_F(LogicalTest, not) {
  EXPECT_EQ(logical_not(true), false);
  EXPECT_EQ(logical_not(false), true);
}

TEST_F(LogicalTest, and_equals) {
  for (const auto &a : {true, false})
    for (const auto &b : {true, false}) {
      bool x = a;
      logical_and_equals(x, b);
      EXPECT_EQ(x, logical_and(a, b));
    }
}

TEST_F(LogicalTest, or_equals) {
  for (const auto &a : {true, false})
    for (const auto &b : {true, false}) {
      bool x = a;
      logical_or_equals(x, b);
      EXPECT_EQ(x, logical_or(a, b));
    }
}

TEST_F(LogicalTest, xor_equals) {
  for (const auto &a : {true, false})
    for (const auto &b : {true, false}) {
      bool x = a;
      logical_xor_equals(x, b);
      EXPECT_EQ(x, logical_xor(a, b));
    }
}
