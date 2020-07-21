// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <chrono>
#include <gtest/gtest.h>

#include "scipp/core/element/arithmetic.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core::element;

class ElementArithmeticTest : public ::testing::Test {
protected:
  const double a = 1.2;
  const double b = 2.3;
  double val = a;

  const int32_t a_int32 = int32_t(1);
  const int64_t a_int64 = int64_t(1);
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point now_copy = now;
};

TEST_F(ElementArithmeticTest, plus_equals) {
  plus_equals(val, b);
  EXPECT_EQ(val, a + b);
  EXPECT_NO_THROW(plus_equals(now, a_int32));
  EXPECT_NO_THROW(plus_equals(now_copy, a_int64));
  EXPECT_TRUE(now == now_copy);
}

TEST_F(ElementArithmeticTest, minus_equals) {
  minus_equals(val, b);
  EXPECT_EQ(val, a - b);
  EXPECT_NO_THROW(plus_equals(now, a_int32));
  EXPECT_NO_THROW(plus_equals(now_copy, a_int64));
  EXPECT_TRUE(now == now_copy);
}

TEST_F(ElementArithmeticTest, times_equals) {
  times_equals(val, b);
  EXPECT_EQ(val, a * b);
}

TEST_F(ElementArithmeticTest, divide_equals) {
  divide_equals(val, b);
  EXPECT_EQ(val, a / b);
}

TEST_F(ElementArithmeticTest, non_in_place) {
  EXPECT_EQ(plus(a, b), a + b);
  EXPECT_EQ(minus(a, b), a - b);
  EXPECT_EQ(times(a, b), a * b);
  EXPECT_EQ(divide(a, b), a / b);
  EXPECT_EQ(plus(now_copy, now), false);
  EXPECT_EQ(minus(now_copy, now), 0);
}

TEST_F(ElementArithmeticTest, unary_minus) { EXPECT_EQ(unary_minus(a), -a); }
