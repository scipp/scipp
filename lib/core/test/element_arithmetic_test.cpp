// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <array>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "scipp/core/element/arithmetic.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/unit.h"

#include "arithmetic_parameters.h"
#include "fix_typed_test_suite_warnings.h"
#include "scipp/core/eigen.h"

using namespace scipp;
using namespace scipp::core::element;

class ElementArithmeticTest : public ::testing::Test {
protected:
  const double a = 1.2;
  const double b = 2.3;
  double val = a;
};

TEST_F(ElementArithmeticTest, add_equals) {
  add_equals(val, b);
  EXPECT_EQ(val, a + b);
}

TEST_F(ElementArithmeticTest, subtract_equals) {
  subtract_equals(val, b);
  EXPECT_EQ(val, a - b);
}

TEST_F(ElementArithmeticTest, multiply_equals) {
  multiply_equals(val, b);
  EXPECT_EQ(val, a * b);
}

TEST_F(ElementArithmeticTest, divide_equals) {
  divide_equals(val, b);
  EXPECT_EQ(val, a / b);
}

TEST_F(ElementArithmeticTest, floor_divide_equals) {
  floor_divide_equals(val, b);
  EXPECT_EQ(val, numeric::floor_divide(a, b));
}

TEST_F(ElementArithmeticTest, mod_equals) {
  mod_equals(val, b);
  EXPECT_EQ(val, mod(a, b));
}

TEST_F(ElementArithmeticTest, non_in_place) {
  EXPECT_EQ(add(a, b), a + b);
  EXPECT_EQ(subtract(a, b), a - b);
  EXPECT_EQ(multiply(a, b), a * b);
  EXPECT_EQ(divide(a, b), a / b);
}

TEST_F(ElementArithmeticTest, negative) { EXPECT_EQ(negative(a), -a); }

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

TEST(ElementArithmeticDivisionTest, inplace_truediv_not_supported) {
  EXPECT_TRUE(no_int_as_first_arg(decltype(divide_equals)::types{}));
}

template <class T> class ElementArithmeticDivisionTest : public testing::Test {
public:
  using Dividend = std::tuple_element_t<0, T>;
  using Divisor = std::tuple_element_t<1, T>;
  // The result is always double if both inputs are integers.
  using TrueQuotient =
      std::conditional_t<std::is_integral_v<Dividend> &&
                             std::is_integral_v<Divisor>,
                         double, std::common_type_t<Dividend, Divisor>>;
  // floor_divide and mod produce integers if both inputs are integers and
  // a float / double otherwise.
  using FloorQuotient = std::common_type_t<Dividend, Divisor>;

  // Helpers to convert values to the correct types.
  constexpr Dividend dividend(Dividend x = Dividend{}) { return x; }
  constexpr Divisor divisor(Divisor x = Divisor{}) { return x; }
  constexpr TrueQuotient true_quotient(TrueQuotient x = TrueQuotient{}) {
    return x;
  }
  constexpr FloorQuotient floor_quotient(FloorQuotient x = FloorQuotient{}) {
    return x;
  }

  struct Params {
    Dividend dividend;
    Divisor divisor;
    TrueQuotient true_quotient;
    FloorQuotient floor_quotient;
    FloorQuotient remainder;
  };

  const auto &params() const {
    if constexpr (std::is_integral_v<Dividend> && std::is_integral_v<Divisor>) {
      return division_params_int_int<Params>;
    } else if constexpr (std::is_floating_point_v<Dividend> &&
                         std::is_integral_v<Divisor>) {
      return division_params_float_int<Params>;
    } else if constexpr (std::is_integral_v<Dividend> &&
                         std::is_floating_point_v<Divisor>) {
      return division_params_int_float<Params>;
    } else {
      return division_params_float_float<Params>;
    }
  }

  template <class Result>
  void expect_eq(const Result actual, const Result expected) {
    if constexpr (std::is_integral_v<Result>) {
      // The result had better be exact.
      EXPECT_EQ(actual, expected);
    } else if constexpr (std::is_same_v<Dividend, float> ||
                         std::is_same_v<Divisor, float>) {
      EXPECT_THAT(static_cast<float>(actual),
                  ::testing::NanSensitiveFloatNear(expected, 1e-5f));
    } else {
      EXPECT_THAT(static_cast<double>(actual),
                  ::testing::NanSensitiveDoubleEq(expected));
    }
  }
};

template <class Tuple, size_t... Indices>
auto DivisionTestTypes_impl(std::index_sequence<Indices...>)
    -> ::testing::Types<std::tuple_element_t<Indices, Tuple>...>;

using DivisionTestTypes =
    decltype(DivisionTestTypes_impl<scipp::core::arithmetic_type_pairs>(
        std::make_index_sequence<
            std::tuple_size_v<scipp::core::arithmetic_type_pairs>>()));

TYPED_TEST_SUITE(ElementArithmeticDivisionTest, DivisionTestTypes);

TEST(ElementArithmeticDivisionTest, true_divide_variance) {
  const ValueAndVariance<double> a(4.2, 0.1);
  const ValueAndVariance<double> b(2.0, 1.2);
  const auto res = divide(a, b);
  EXPECT_DOUBLE_EQ(res.value, 2.1);
  EXPECT_DOUBLE_EQ(res.variance, 1.3479999999999999);
}

TYPED_TEST(ElementArithmeticDivisionTest, true_divide) {
  EXPECT_TRUE(
      (std::is_same_v<decltype(divide(this->dividend(), this->divisor())),
                      typename TestFixture::TrueQuotient>));
  for (const auto p : this->params()) {
    this->expect_eq(divide(p.dividend, p.divisor), p.true_quotient);
  }
}

TYPED_TEST(ElementArithmeticDivisionTest, floor_divide) {
  EXPECT_TRUE(
      (std::is_same_v<decltype(floor_divide(this->dividend(), this->divisor())),
                      typename TestFixture::FloorQuotient>));

  for (const auto p : this->params()) {
    this->expect_eq(floor_divide(p.dividend, p.divisor), p.floor_quotient);
  }
}

TYPED_TEST(ElementArithmeticDivisionTest, remainder) {
  EXPECT_TRUE((std::is_same_v<decltype(mod(this->dividend(), this->divisor())),
                              typename TestFixture::FloorQuotient>));

  for (const auto p : this->params()) {
    this->expect_eq(mod(p.dividend, p.divisor), p.remainder);
  }
}

TEST(ElementArithmeticDivisionTest, units) {
  EXPECT_EQ(divide(sc_units::m, sc_units::s), sc_units::m / sc_units::s);
  EXPECT_EQ(floor_divide(sc_units::m, sc_units::s), sc_units::m / sc_units::s);
  EXPECT_EQ(mod(sc_units::m, sc_units::m), sc_units::m);
  EXPECT_THROW(mod(sc_units::m, sc_units::s), except::UnitError);
}

class ElementNanArithmeticTest : public ::testing::Test {
protected:
  double x = 1.0;
  double y = 2.0;
  double dNaN = std::numeric_limits<double>::quiet_NaN();
};

TEST_F(ElementNanArithmeticTest, add_equals) {
  auto expected = x + y;
  nan_add_equals(x, y);
  EXPECT_EQ(expected, x);
}

TEST_F(ElementNanArithmeticTest, add_equals_with_rhs_nan) {
  auto expected = x + 0;
  nan_add_equals(x, dNaN);
  EXPECT_EQ(expected, x);
}
TEST_F(ElementNanArithmeticTest, add_equals_with_lhs_nan) {
  auto expected = y + 0;
  auto lhs = dNaN;
  nan_add_equals(lhs, y);
  EXPECT_EQ(expected, lhs);
}
TEST_F(ElementNanArithmeticTest, add_equals_with_both_nan) {
  auto lhs = dNaN;
  nan_add_equals(lhs, dNaN);
  EXPECT_EQ(0, lhs);
}
TEST_F(ElementNanArithmeticTest, add_equals_with_rhs_nan_ValueAndVariance) {
  ValueAndVariance<double> asNaN{dNaN, 0};
  ValueAndVariance<double> z{1, 0};
  auto expected = z + ValueAndVariance<double>{0, 0};
  nan_add_equals(z, asNaN);
  EXPECT_EQ(expected, z);
}
TEST_F(ElementNanArithmeticTest, add_equals_with_lhs_nan_rhs_int) {
  auto lhs = dNaN;
  nan_add_equals(lhs, 1);
  EXPECT_EQ(1.0, lhs);
}
TEST_F(ElementNanArithmeticTest, add_equals_with_rhs_int_lhs_nan) {
  auto lhs = 1;
  nan_add_equals(lhs, dNaN);
  EXPECT_EQ(1, lhs);
}
TEST_F(ElementNanArithmeticTest, add_equals_with_rhs_int_lhs_int) {
  auto lhs = 1;
  nan_add_equals(lhs, 2);
  EXPECT_EQ(3, lhs);
}

class ElementMatrixMatrixArithmeticTest : public ::testing::Test {
protected:
  Eigen::Matrix3d x = Eigen::Matrix3d::Constant(3, 3, 1);
  Eigen::Matrix3d y = Eigen::Matrix3d::Constant(3, 3, 2);

public:
  ElementMatrixMatrixArithmeticTest() {
    y.col(0) = Eigen::Vector3d::LinSpaced(3, 0, 2);
  }
};

TEST_F(ElementMatrixMatrixArithmeticTest, multiply_equals) {
  Eigen::Matrix3d expected = x * y;
  multiply_equals(x, y);
  EXPECT_EQ(expected, x);
}

TEST_F(ElementMatrixMatrixArithmeticTest, multiply_equals_self_assign) {
  Eigen::Matrix3d expected = x * x.eval();
  multiply_equals(x, x);
  EXPECT_EQ(expected, x);
}
