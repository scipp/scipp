// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
};

TEST_F(ElementArithmeticTest, plus_equals) {
  plus_equals(val, b);
  EXPECT_EQ(val, a + b);
}

TEST_F(ElementArithmeticTest, minus_equals) {
  minus_equals(val, b);
  EXPECT_EQ(val, a - b);
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
}

TEST_F(ElementArithmeticTest, unary_minus) { EXPECT_EQ(unary_minus(a), -a); }

TEST(ElementArithmeticIntegerDivisionTest, truediv_32bit) {
  const int32_t a = 2;
  const int32_t b = 3;
  // 32 bit ints give double not float, consistent with numpy
  EXPECT_EQ(divide(a, b), 2.0 / 3.0);
}

TEST(ElementArithmeticIntegerDivisionTest, truediv_64bit) {
  const int64_t a = 2;
  const int64_t b = 3;
  EXPECT_EQ(divide(a, b), 2.0 / 3.0);
}

template <class T> struct int_as_first_arg : std::false_type {};
template <> struct int_as_first_arg<int64_t> : std::true_type {};
template <> struct int_as_first_arg<int32_t> : std::true_type {};
template <class A, class B>
struct int_as_first_arg<std::tuple<A, B>> : int_as_first_arg<A> {};

template <class... Ts> auto no_int_as_first_arg(const std::tuple<Ts...> &) {
  return !(int_as_first_arg<Ts>::value || ...);
}

TEST(ElementArithmeticIntegerDivisionTest, inplace_truediv_not_supported) {
  EXPECT_TRUE(no_int_as_first_arg(decltype(divide_equals)::types{}));
}
