// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

TEST(ElementArithmeticIntegerDivisionTest, truediv) {
  // 32 bit ints gives double not float, consistent with numpy
  EXPECT_TRUE((std::is_same_v<decltype(divide(int32_t{}, int32_t{})), double>));
  EXPECT_TRUE((std::is_same_v<decltype(divide(int64_t{}, int64_t{})), double>));
}

TEST(ElementArithmeticIntegerDivisionTest, truediv_32bit) {
  const int32_t a = 2;
  const int32_t b = 3;
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

TEST(ElementArithmeticIntegerDivisionTest, mod) {
  EXPECT_EQ(mod(-3, 3), 0);
  EXPECT_EQ(mod(-2, 3), 1);
  EXPECT_EQ(mod(-1, 3), 2);
  EXPECT_EQ(mod(0, 3), 0);
  EXPECT_EQ(mod(1, 3), 1);
  EXPECT_EQ(mod(2, 3), 2);
  EXPECT_EQ(mod(3, 3), 0);
  EXPECT_EQ(mod(4, 3), 1);
}

TEST(ElementArithmeticIntegerDivisionTest, mod_equals) {
  constexpr auto check_mod = [](auto a, auto b, auto expected) {
    mod_equals(a, b);
    EXPECT_EQ(a, expected);
  };
  check_mod(-2, 3, 1);
  check_mod(-1, 3, 2);
  check_mod(0, 3, 0);
  check_mod(1, 3, 1);
  check_mod(2, 3, 2);
  check_mod(3, 3, 0);
  check_mod(4, 3, 1);
}

class ElementNanArithmeticTest : public ::testing::Test {
protected:
  double x = 1.0;
  double y = 2.0;
  double dNaN = std::numeric_limits<double>::quiet_NaN();
};

TEST_F(ElementNanArithmeticTest, plus_equals) {
  auto expected = x + y;
  nan_plus_equals(x, y);
  EXPECT_EQ(expected, x);
}

TEST_F(ElementNanArithmeticTest, plus_equals_with_rhs_nan) {
  auto expected = x + 0;
  nan_plus_equals(x, dNaN);
  EXPECT_EQ(expected, x);
}
TEST_F(ElementNanArithmeticTest, plus_equals_with_lhs_nan) {
  auto expected = y + 0;
  auto lhs = dNaN;
  nan_plus_equals(lhs, y);
  EXPECT_EQ(expected, lhs);
}
TEST_F(ElementNanArithmeticTest, plus_equals_with_both_nan) {
  auto lhs = dNaN;
  nan_plus_equals(lhs, dNaN);
  EXPECT_EQ(0, lhs);
}
TEST_F(ElementNanArithmeticTest, plus_equals_with_rhs_nan_ValueAndVariance) {
  ValueAndVariance<double> asNaN{dNaN, 0};
  ValueAndVariance<double> z{1, 0};
  auto expected = z + ValueAndVariance<double>{0, 0};
  nan_plus_equals(z, asNaN);
  EXPECT_EQ(expected, z);
}
TEST_F(ElementNanArithmeticTest, plus_equals_with_lhs_nan_rhs_int) {
  auto lhs = dNaN;
  nan_plus_equals(lhs, 1);
  EXPECT_EQ(1.0, lhs);
}
TEST_F(ElementNanArithmeticTest, plus_equals_with_rhs_int_lhs_nan) {
  auto lhs = 1;
  nan_plus_equals(lhs, dNaN);
  EXPECT_EQ(1, lhs);
}
TEST_F(ElementNanArithmeticTest, plus_equals_with_rhs_int_lhs_int) {
  auto lhs = 1;
  nan_plus_equals(lhs, 2);
  EXPECT_EQ(3, lhs);
}
