// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <Eigen/Geometry>
#include <gtest/gtest.h>
#include <vector>

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

#include "scipp/common/constants.h"
#include "scipp/core/element/math.h"
#include "scipp/variable/except.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

template <typename T>
class VariableScalarOperatorTest : public ::testing::Test {
public:
  Variable variable{makeVariable<T>(Dims{Dim::X}, Shape{1}, Values{10})};
  const T scalar{2};

  T value() const { return this->variable.template values<T>()[0]; }
};

using ScalarTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(VariableScalarOperatorTest, ScalarTypes);

TYPED_TEST(VariableScalarOperatorTest, plus_equals) {
  this->variable += this->scalar * sc_units::one;
  EXPECT_EQ(this->value(), 12);
}

TYPED_TEST(VariableScalarOperatorTest, minus_equals) {
  this->variable -= this->scalar * sc_units::one;
  EXPECT_EQ(this->value(), 8);
}

TYPED_TEST(VariableScalarOperatorTest, times_equals) {
  this->variable *= this->scalar * sc_units::one;
  EXPECT_EQ(this->value(), 20);
}

TYPED_TEST(VariableScalarOperatorTest, divide_equals) {
  if (this->variable.dtype() == dtype<double> ||
      this->variable.dtype() == dtype<float>) {
    this->variable /= this->scalar * sc_units::one;
    EXPECT_EQ(this->value(), 5);
  } else {
    EXPECT_THROW(this->variable /= this->scalar * sc_units::one,
                 except::TypeError);
  }
}

TEST(Variable, operator_unary_minus) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                      Values{1.1, 2.2});
  const auto expected = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                             sc_units::m, Values{-1.1, -2.2});
  auto b = -a;
  EXPECT_EQ(b, expected);
}

TEST(VariableView, unary_minus) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                      Values{1.1, 2.2});
  const auto expected =
      makeVariable<double>(Dims(), Shape(), sc_units::m, Values{-2.2});
  auto b = -a.slice({Dim::X, 1});
  EXPECT_EQ(b, expected);
}

TEST(Variable, operator_plus_equal) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  ASSERT_NO_THROW(a += a);
  EXPECT_EQ(a.values<double>()[0], 2.2);
  EXPECT_EQ(a.values<double>()[1], 4.4);
}

TEST(Variable, operator_plus_equal_automatic_broadcast_of_rhs) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  auto fewer_dimensions = makeVariable<double>(Values{1.0});

  ASSERT_NO_THROW(a += fewer_dimensions);
  EXPECT_EQ(a.values<double>()[0], 2.1);
  EXPECT_EQ(a.values<double>()[1], 3.2);
}

TEST(Variable, operator_plus_equal_transpose) {
  auto a = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 2}, sc_units::m,
                                Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  auto transpose =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3}, sc_units::m,
                           Values{1.0, 3.0, 5.0, 2.0, 4.0, 6.0});

  EXPECT_NO_THROW(a += transpose);
  ASSERT_EQ(a,
            makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 2}, sc_units::m,
                                 Values{2.0, 4.0, 6.0, 8.0, 10.0, 12.0}));
}

TEST(Variable, operator_plus_equal_different_dimensions) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  auto different_dimensions =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.1, 2.2});
  EXPECT_THROW_MSG(a += different_dimensions, std::runtime_error,
                   "Expected (x: 2) to include (y: 2).");
}

TEST(Variable, operator_plus_equal_different_unit) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  auto different_unit = copy(a);
  different_unit.setUnit(sc_units::m);
  EXPECT_THROW(a += different_unit, except::UnitError);
}

TEST(Variable, operator_plus_equal_non_arithmetic_type) {
  auto a = makeVariable<std::string>(Dims{Dim::X}, Shape{1},
                                     Values{std::string("test")});
  EXPECT_THROW(a += a, except::TypeError);
}

TEST(Variable, operator_plus_equal_time_type) {
  using time_point = scipp::core::time_point;
  auto a = makeVariable<time_point>(Shape{1}, sc_units::Unit{sc_units::ns},
                                    Values{time_point{2}});
  EXPECT_THROW(a += static_cast<float>(1.0) * sc_units::ns, except::TypeError);
  EXPECT_NO_THROW(a += static_cast<int64_t>(1) * sc_units::ns);
  EXPECT_NO_THROW(a += static_cast<int32_t>(2) * sc_units::ns);
  EXPECT_EQ(a, makeVariable<time_point>(Shape{1}, sc_units::Unit{sc_units::ns},
                                        Values{time_point{5}}));
}

TEST(Variable, operator_minus_equal_time_type) {
  using time_point = scipp::core::time_point;
  auto a = makeVariable<time_point>(Shape{1}, sc_units::Unit{sc_units::ns},
                                    Values{time_point{10}});
  EXPECT_THROW(a -= static_cast<float>(1.0) * sc_units::ns, except::TypeError);
  EXPECT_NO_THROW(a -= static_cast<int64_t>(1) * sc_units::ns);
  EXPECT_NO_THROW(a -= static_cast<int32_t>(2) * sc_units::ns);
  EXPECT_EQ(a, makeVariable<time_point>(Shape{1}, sc_units::Unit{sc_units::ns},
                                        Values{time_point{7}}));
}

TEST(Variable, operator_plus_equal_different_variables_different_element_type) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  auto b = makeVariable<int64_t>(Dims{Dim::X}, Shape{1}, Values{2});
  EXPECT_NO_THROW(a += b);
}

TEST(Variable, operator_plus_equal_different_variables_same_element_type) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{2.0});
  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.values<double>()[0], 3.0);
}

TEST(Variable, operator_plus_equal_scalar) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  EXPECT_NO_THROW(a += 1.0 * sc_units::one);
  EXPECT_EQ(a.values<double>()[0], 2.1);
  EXPECT_EQ(a.values<double>()[1], 3.2);
}

TEST(Variable, operator_plus_equal_custom_type) {
  auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1f, 2.2f});

  EXPECT_NO_THROW(a += a);
  EXPECT_EQ(a.values<float>()[0], 2.2f);
  EXPECT_EQ(a.values<float>()[1], 4.4f);
}

TEST(Variable, operator_plus_unit_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0},
                                Variances{3.0, 4.0});
  a.setUnit(sc_units::m);
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0},
                                Variances{3.0, 4.0});
  b.setUnit(sc_units::s);
  ASSERT_ANY_THROW_DISCARD(a + b);
  b.setUnit(sc_units::m);
  ASSERT_NO_THROW_DISCARD(a + b);
}

TEST(Variable, operator_plus_eigen_type) {
  const auto var = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2},
      Values{Eigen::Vector3d{1.0, 2.0, 3.0}, Eigen::Vector3d{0.1, 0.2, 0.3}});
  const auto expected = makeVariable<Eigen::Vector3d>(
      Dims(), Shape(), Values{Eigen::Vector3d{1.1, 2.2, 3.3}});

  const auto result = var.slice({Dim::X, 0}) + var.slice({Dim::X, 1});

  EXPECT_EQ(result, expected);
}

TEST(Variable, operator_times_equal) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                Values{2.0, 3.0});

  EXPECT_EQ(a.unit(), sc_units::m);
  EXPECT_NO_THROW(a *= a);
  EXPECT_EQ(a.values<double>()[0], 4.0);
  EXPECT_EQ(a.values<double>()[1], 9.0);
  EXPECT_EQ(a.unit(), sc_units::m * sc_units::m);
}

TEST(Variable, operator_times_equal_scalar) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                Values{2.0, 3.0});

  EXPECT_EQ(a.unit(), sc_units::m);
  EXPECT_NO_THROW(a *= 2.0 * sc_units::one);
  EXPECT_EQ(a.values<double>()[0], 4.0);
  EXPECT_EQ(a.values<double>()[1], 6.0);
  EXPECT_EQ(a.unit(), sc_units::m);
}

TEST(Variable, operator_plus_equal_unit_fail_integrity) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                sc_units::Unit(sc_units::m * sc_units::m),
                                Values{2.0, 3.0});
  const auto expected(a);

  ASSERT_THROW(a += a * a, std::runtime_error);
  EXPECT_EQ(a, expected);
}

TEST(Variable, operator_times_can_broadcast) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.5, 1.5});
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{2.0, 3.0});

  auto ab = a * b;
  auto reference = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                        Values{1.0, 1.5, 3.0, 4.5});
  EXPECT_EQ(ab, reference);
}

TEST(Variable, operator_divide_equal) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 3.0});
  auto b = makeVariable<double>(Values{2.0});
  b.setUnit(sc_units::m);

  EXPECT_NO_THROW(a /= b);
  EXPECT_EQ(a.values<double>()[0], 1.0);
  EXPECT_EQ(a.values<double>()[1], 1.5);
  EXPECT_EQ(a.unit(), sc_units::one / sc_units::m);
}

TEST(Variable, operator_divide_equal_self) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                Values{2.0, 3.0});

  EXPECT_EQ(a.unit(), sc_units::m);
  EXPECT_NO_THROW(a /= a);
  EXPECT_EQ(a.values<double>()[0], 1.0);
  EXPECT_EQ(a.values<double>()[1], 1.0);
  EXPECT_EQ(a.unit(), sc_units::one);
}

TEST(Variable, operator_divide_equal_scalar) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                Values{2.0, 4.0});

  EXPECT_EQ(a.unit(), sc_units::m);
  EXPECT_NO_THROW(a /= 2.0 * sc_units::one);
  EXPECT_EQ(a.values<double>()[0], 1.0);
  EXPECT_EQ(a.values<double>()[1], 2.0);
  EXPECT_EQ(a.unit(), sc_units::m);
}

TEST(Variable, operator_divide_scalar_double) {
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                      Values{2.0, 4.0});
  const auto result = 1.111 * sc_units::one / a;
  EXPECT_EQ(result.values<double>()[0], 1.111 / 2.0);
  EXPECT_EQ(result.values<double>()[1], 1.111 / 4.0);
  EXPECT_EQ(result.unit(), sc_units::one / sc_units::m);
}

TEST(Variable, operator_divide_scalar_float) {
  const auto a = makeVariable<float>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                     Values{2.0, 4.0});
  const auto result = 1.111f * sc_units::one / a;
  EXPECT_EQ(result.values<float>()[0], 1.111f / 2.0f);
  EXPECT_EQ(result.values<float>()[1], 1.111f / 4.0f);
  EXPECT_EQ(result.unit(), sc_units::one / sc_units::m);
}

TEST(Variable, operator_allowed_types) {
  auto i32 = makeVariable<int32_t>(Values{10});
  auto i64 = makeVariable<int64_t>(Values{10});
  auto f = makeVariable<float>(Values{0.5f});
  auto d = makeVariable<double>(Values{0.5});

  /* Can operate on higher precision from lower precision */
  EXPECT_NO_THROW(i64 += i32);
  EXPECT_NO_THROW(d += f);

  /* Can operate on lower precision from higher precision */
  EXPECT_NO_THROW(i32 += i64);
  EXPECT_NO_THROW(f += d);

  /* Expect promotion to double if one parameter is double */
  EXPECT_EQ(dtype<double>, (f + d).dtype());
  EXPECT_EQ(dtype<double>, (d + f).dtype());
}

TEST(VariableView, minus_equals_failures) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});

  EXPECT_THROW_MSG(var -= var.slice({Dim::X, 0, 1}), std::runtime_error,
                   "Expected (x: 2, y: 2) to include (x: 1, y: 2).");
}

TEST(VariableView, self_overlapping_view_operation) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});

  var -= var.slice({Dim::Y, 0});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  // This is the critical part: After subtracting for y=0 the view points to
  // data containing 0.0, so subsequently the subtraction would have no effect
  // if self-overlap was not taken into account by the implementation.
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
}

TEST(VariableView, minus_equals_slice_const_outer) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  const auto copy = variable::copy(var);

  var -= copy.slice({Dim::Y, 0});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
  // cppcheck-suppress unreadVariable  # Read through `data`.
  var -= copy.slice({Dim::Y, 1});
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -4.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableView, minus_equals_slice_outer) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  auto copy = variable::copy(var);

  var -= copy.slice({Dim::Y, 0});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
  // cppcheck-suppress unreadVariable  # Read through `data`.
  var -= copy.slice({Dim::Y, 1});
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -4.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableView, minus_equals_slice_inner) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  auto copy = variable::copy(var);

  var -= copy.slice({Dim::X, 0});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 1.0);
  EXPECT_EQ(data[2], 0.0);
  EXPECT_EQ(data[3], 1.0);
  // cppcheck-suppress unreadVariable  # Read through `data`.
  var -= copy.slice({Dim::X, 1});
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -1.0);
  EXPECT_EQ(data[2], -4.0);
  EXPECT_EQ(data[3], -3.0);
}

TEST(VariableView, minus_equals_slice_of_slice) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  auto copy = variable::copy(var);

  var -= copy.slice({Dim::X, 1}).slice({Dim::Y, 1});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 0.0);
}

TEST(VariableView, minus_equals_nontrivial_slices) {
  auto source = makeVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{3, 3},
      Values{11.0, 12.0, 13.0, 21.0, 22.0, 23.0, 31.0, 32.0, 33.0});
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});
    target -= source.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], -21.0);
    EXPECT_EQ(data[3], -22.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});
    target -= source.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], -12.0);
    EXPECT_EQ(data[1], -13.0);
    EXPECT_EQ(data[2], -22.0);
    EXPECT_EQ(data[3], -23.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});
    target -= source.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], -21.0);
    EXPECT_EQ(data[1], -22.0);
    EXPECT_EQ(data[2], -31.0);
    EXPECT_EQ(data[3], -32.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});
    target -= source.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], -22.0);
    EXPECT_EQ(data[1], -23.0);
    EXPECT_EQ(data[2], -32.0);
    EXPECT_EQ(data[3], -33.0);
  }
}

TEST(VariableView, slice_inner_minus_equals) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});

  var.slice({Dim::X, 0}) -= var.slice({Dim::X, 1});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], -1.0);
  EXPECT_EQ(data[1], 2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableView, slice_outer_minus_equals) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});

  var.slice({Dim::Y, 0}) -= var.slice({Dim::Y, 1});
  const auto data = var.values<double>();
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], 3.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableView, nontrivial_slice_minus_equals) {
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       Values{11.0, 12.0, 21.0, 22.0});
    target.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}) -= source;
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -21.0);
    EXPECT_EQ(data[4], -22.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       Values{11.0, 12.0, 21.0, 22.0});
    target.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}) -= source;
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], -11.0);
    EXPECT_EQ(data[2], -12.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -21.0);
    EXPECT_EQ(data[5], -22.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       Values{11.0, 12.0, 21.0, 22.0});
    target.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}) -= source;
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -11.0);
    EXPECT_EQ(data[4], -12.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], -21.0);
    EXPECT_EQ(data[7], -22.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       Values{11.0, 12.0, 21.0, 22.0});
    target.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}) -= source;
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -11.0);
    EXPECT_EQ(data[5], -12.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], -21.0);
    EXPECT_EQ(data[8], -22.0);
  }
}

TEST(VariableView, nontrivial_slice_minus_equals_slice) {
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source =
        makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                             Values{666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target.slice({Dim::X, 0, 2}).slice({Dim::Y, 0, 2}) -=
        source.slice({Dim::X, 1, 3});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -21.0);
    EXPECT_EQ(data[4], -22.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source =
        makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                             Values{666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target.slice({Dim::X, 1, 3}).slice({Dim::Y, 0, 2}) -=
        source.slice({Dim::X, 1, 3});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], -11.0);
    EXPECT_EQ(data[2], -12.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -21.0);
    EXPECT_EQ(data[5], -22.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source =
        makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                             Values{666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target.slice({Dim::X, 0, 2}).slice({Dim::Y, 1, 3}) -=
        source.slice({Dim::X, 1, 3});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -11.0);
    EXPECT_EQ(data[4], -12.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], -21.0);
    EXPECT_EQ(data[7], -22.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
    auto source =
        makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                             Values{666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3}) -=
        source.slice({Dim::X, 1, 3});
    const auto data = target.values<double>();
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -11.0);
    EXPECT_EQ(data[5], -12.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], -21.0);
    EXPECT_EQ(data[8], -22.0);
  }
}

TEST(VariableView, slice_minus_lower_dimensional) {
  auto target = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});
  auto source = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  EXPECT_EQ(target.slice({Dim::Y, 1, 2}).dims(),
            (Dimensions{{Dim::Y, 1}, {Dim::X, 2}}));

  target.slice({Dim::Y, 1, 2}) -= source;

  const auto data = target.values<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableView, slice_binary_operations) {
  auto v = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                Values{1, 2, 3, 4});
  // Note: There does not seem to be a way to test whether this is using the
  // operators that convert the second argument to Variable (it should not), or
  // keep it as a view. See variable_benchmark.cpp for an attempt to verify
  // this.
  auto sum = v.slice({Dim::X, 0}) + v.slice({Dim::X, 1});
  auto difference = v.slice({Dim::X, 0}) - v.slice({Dim::X, 1});
  auto product = v.slice({Dim::X, 0}) * v.slice({Dim::X, 1});
  auto ratio = v.slice({Dim::X, 0}) / v.slice({Dim::X, 1});
  EXPECT_TRUE(equals(sum.values<double>(), {3, 7}));
  EXPECT_TRUE(equals(difference.values<double>(), {-1, -1}));
  EXPECT_TRUE(equals(product.values<double>(), {2, 12}));
  EXPECT_TRUE(equals(ratio.values<double>(), {1.0 / 2.0, 3.0 / 4.0}));
}

TEST(Variable, non_in_place_scalar_operations) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});

  auto sum = var + 1.0 * sc_units::one;
  EXPECT_TRUE(equals(sum.values<double>(), {2, 3}));
  sum = 2.0 * sc_units::one + var;
  EXPECT_TRUE(equals(sum.values<double>(), {3, 4}));

  auto diff = var - 1.0 * sc_units::one;
  EXPECT_TRUE(equals(diff.values<double>(), {0, 1}));
  diff = 2.0 * sc_units::one - var;
  EXPECT_TRUE(equals(diff.values<double>(), {1, 0}));

  auto prod = var * (2.0 * sc_units::one);
  EXPECT_TRUE(equals(prod.values<double>(), {2, 4}));
  prod = 3.0 * sc_units::one * var;
  EXPECT_TRUE(equals(prod.values<double>(), {3, 6}));

  auto ratio = var / (2.0 * sc_units::one);
  EXPECT_TRUE(equals(ratio.values<double>(), {1.0 / 2.0, 1.0}));
  ratio = 3.0 * sc_units::one / var;
  EXPECT_TRUE(equals(ratio.values<double>(), {3.0, 1.5}));
}

TEST(VariableView, scalar_operations) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                  Values{11, 12, 13, 21, 22, 23});

  var.slice({Dim::X, 0}) += 1.0 * sc_units::one;
  EXPECT_TRUE(equals(var.values<double>(), {12, 12, 13, 22, 22, 23}));
  var.slice({Dim::Y, 1}) += 1.0 * sc_units::one;
  EXPECT_TRUE(equals(var.values<double>(), {12, 12, 13, 23, 23, 24}));
  var.slice({Dim::X, 1, 3}) += 1.0 * sc_units::one;
  EXPECT_TRUE(equals(var.values<double>(), {12, 13, 14, 23, 24, 25}));
  var.slice({Dim::X, 1}) -= 1.0 * sc_units::one;
  EXPECT_TRUE(equals(var.values<double>(), {12, 12, 14, 23, 23, 25}));
  var.slice({Dim::X, 2}) *= 0.0 * sc_units::one;
  EXPECT_TRUE(equals(var.values<double>(), {12, 12, 0, 23, 23, 0}));
  var.slice({Dim::Y, 0}) /= 2.0 * sc_units::one;
  EXPECT_TRUE(equals(var.values<double>(), {6, 6, 0, 23, 23, 0}));
}

TEST(VariableTest, binary_op_with_variance) {
  const auto var = makeVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 3}, Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
      Variances{0.1, 0.2, 0.3, 0.4, 0.5, 0.6});
  const auto sum = makeVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 3}, Values{2.0, 4.0, 6.0, 8.0, 10.0, 12.0},
      Variances{0.2, 0.4, 0.6, 0.8, 1.0, 1.2});
  auto tmp = var + copy(var); // copy to avoid correlation detection
  EXPECT_TRUE(tmp.has_variances());
  EXPECT_EQ(tmp.variances<double>()[0], 0.2);
  EXPECT_EQ(tmp, sum);

  tmp = var * sum;
  EXPECT_EQ(tmp.variances<double>()[0], 0.1 * 2.0 * 2.0 + 0.2 * 1.0 * 1.0);
}

TEST(VariableTest, divide_with_variance) {
  // Note the 0.0: With a wrong implementation the resulting variance is INF.
  const auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0, 0.0},
                                      Variances{0.1, 0.1});
  const auto b = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{3.0, 3.0},
                                      Variances{0.2, 0.2});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2.0 / 3.0, 0.0},
                           Variances{(0.1 / (2.0 * 2.0) + 0.2 / (3.0 * 3.0)) *
                                         (2.0 / 3.0) * (2.0 / 3.0),
                                     /* (0.1 / (0.0 * 0.0) + 0.2 / (3.0
                                      * 3.0)) * (0.0 / 3.0) * (0.0 / 3.0)
                                      naively, but if we take the limit... */
                                     0.1 / (3.0 * 3.0)});
  const auto q = a / b;
  EXPECT_DOUBLE_EQ(q.values<double>()[0], expected.values<double>()[0]);
  EXPECT_DOUBLE_EQ(q.values<double>()[1], expected.values<double>()[1]);
  EXPECT_DOUBLE_EQ(q.variances<double>()[0], expected.variances<double>()[0]);
  EXPECT_DOUBLE_EQ(q.variances<double>()[1], expected.variances<double>()[1]);
}

TEST(VariableTest, boolean_or) {
  const auto a = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, true, false, true});
  const auto b = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, false, true, true});
  const auto expected = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                           Values{false, true, true, true});

  const auto result = a | b;

  EXPECT_EQ(result, expected);
}

TEST(VariableTest, boolean_or_equals) {
  auto a = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                              Values{false, true, false, true});
  const auto b = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, false, true, true});
  a |= b;
  const auto expected = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                           Values{false, true, true, true});

  EXPECT_EQ(a, expected);
}

TEST(VariableTest, boolean_and_equals) {
  auto a = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                              Values{false, true, false, true});
  const auto b = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, false, true, true});
  a &= b;
  const auto expected = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                           Values{false, false, false, true});

  EXPECT_EQ(a, expected);
}

TEST(VariableTest, boolean_and) {
  const auto a = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, true, false, true});
  const auto b = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, false, true, true});
  const auto expected = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                           Values{false, false, false, true});

  const auto result = a & b;

  EXPECT_EQ(result, expected);
}

TEST(VariableTest, boolean_xor_equals) {
  auto a = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                              Values{false, true, false, true});
  const auto b = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, false, true, true});
  a ^= b;
  const auto expected = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                           Values{false, true, true, false});

  EXPECT_EQ(a, expected);
}

TEST(VariableTest, boolean_xor) {
  const auto a = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, true, false, true});
  const auto b = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                    Values{false, false, true, true});
  const auto expected = makeVariable<bool>(Dims{Dim::X}, Shape{4},
                                           Values{false, true, true, false});
  const auto result = a ^ b;

  EXPECT_EQ(result, expected);
}

TEST(VariableTest, zip_positions) {
  const Variable x = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                          Values{1, 2, 3});
  auto positions = geometry::position(x, x, x);
  auto values = positions.values<Eigen::Vector3d>();
  EXPECT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], (Eigen::Vector3d{1, 1, 1}));
  EXPECT_EQ(values[1], (Eigen::Vector3d{2, 2, 2}));
  EXPECT_EQ(values[2], (Eigen::Vector3d{3, 3, 3}));
}

TEST(VariableTest, rotate) {
  Eigen::Vector3d vec1(1, 2, 3);
  Eigen::Vector3d vec2(4, 5, 6);
  auto vec = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                           Values{vec1, vec2});
  Eigen::Quaterniond rot1(1.1, 2.2, 3.3, 4.4);
  Eigen::Quaterniond rot2(5.5, 6.6, 7.7, 8.8);
  rot1.normalize();
  rot2.normalize();
  const Variable rot = makeVariable<Eigen::Matrix3d>(
      Dims{Dim::X}, Shape{2}, sc_units::one,
      Values{rot1.toRotationMatrix(), rot2.toRotationMatrix()});
  auto vec_new = rot * vec;
  const auto rotated = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2}, sc_units::m,
      Values{rot1.toRotationMatrix() * vec1, rot2.toRotationMatrix() * vec2});
  EXPECT_EQ(vec_new, rotated);
}

TEST(VariableTest, combine_translations) {
  Eigen::Vector3d translation1(1, 2, 3);
  Eigen::Vector3d translation2(4, 5, 6);

  auto trans1 = makeVariable<scipp::core::Translation>(
      Dims{Dim::X}, Shape{1}, sc_units::m,
      Values{scipp::core::Translation(translation1)});
  auto trans2 = makeVariable<scipp::core::Translation>(
      Dims{Dim::X}, Shape{1}, sc_units::m,
      Values{scipp::core::Translation(translation2)});

  Eigen::Vector3d expected(5, 7, 9);
  auto expected_var = makeVariable<scipp::core::Translation>(
      Dims{Dim::X}, Shape{1}, sc_units::m,
      Values{scipp::core::Translation(expected)});

  EXPECT_EQ(trans1 * trans2, expected_var);
}

TEST(VariableTest, combine_translation_and_rotation) {
  const Eigen::Vector3d translation(1, 2, 3);
  Eigen::Quaterniond rotation;
  rotation = Eigen::AngleAxisd(pi<double>, Eigen::Vector3d::UnitX());

  const Variable translation_var = makeVariable<scipp::core::Translation>(
      Dims{Dim::X}, Shape{1}, sc_units::m,
      Values{scipp::core::Translation(translation)});
  const Variable rotation_var = makeVariable<scipp::core::Quaternion>(
      Dims{Dim::X}, Shape{1}, sc_units::one,
      Values{scipp::core::Quaternion(rotation)});

  // Translation and rotation -> affine
  const Eigen::Affine3d expected(Eigen::Translation<double, 3>(translation) *
                                 rotation);
  const Variable expected_var = makeVariable<Eigen::Affine3d>(
      Dims{Dim::X}, Shape{1}, sc_units::m, Values{expected});

  EXPECT_EQ(translation_var * rotation_var, expected_var);
}

TEST(VariableTest, combine_rotations) {
  Eigen::Quaterniond rotation1;
  rotation1 = Eigen::AngleAxisd(pi<double> / 2.0, Eigen::Vector3d::UnitX());

  Eigen::Quaterniond rotation2;
  rotation2 = Eigen::AngleAxisd(pi<double> / 2.0, Eigen::Vector3d::UnitX());

  const Variable rotation1_var = makeVariable<scipp::core::Quaternion>(
      Dims{Dim::X}, Shape{1}, sc_units::one,
      Values{scipp::core::Quaternion(rotation1)});

  const Variable rotation2_var = makeVariable<scipp::core::Quaternion>(
      Dims{Dim::X}, Shape{1}, sc_units::one,
      Values{scipp::core::Quaternion(rotation2)});

  // Rotation and rotation -> rotation
  const Eigen::Quaterniond expected(rotation1 * rotation2);
  const Variable expected_var = makeVariable<scipp::core::Quaternion>(
      Dims{Dim::X}, Shape{1}, sc_units::one,
      Values{scipp::core::Quaternion(expected)});

  EXPECT_EQ(rotation1_var * rotation2_var, expected_var);
}

class ApplyTransformTest : public ::testing::Test {
public:
  Variable makeTransformVar(const sc_units::Unit unit) {
    Eigen::Vector3d rotation_axis(1, 0, 0);
    Eigen::Affine3d t(Eigen::AngleAxisd(pi<double> / 2.0, rotation_axis));

    return makeVariable<Eigen::Affine3d>(Dims{Dim::X}, Shape{1}, unit,
                                         Values{t});
  }

  Variable makeVectorVar(const sc_units::Unit unit) {
    Eigen::Vector3d eigen_vec(1, 2, 3);
    return makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{1}, unit,
                                         Values{eigen_vec});
  }
};

TEST_F(ApplyTransformTest, apply_transform_to_vector) {
  auto transformed = makeTransformVar(sc_units::m) * makeVectorVar(sc_units::m);

  Eigen::Vector3d expected(1, -3, 2);
  EXPECT_EQ(transformed,
            makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{1}, sc_units::m,
                                          Values{expected}));
}

TEST_F(ApplyTransformTest, apply_transform_to_vector_with_different_units) {
  // Even though the transform and vector both have units of length, we don't
  // allow this application of a transform. The units must match exactly as the
  // transform may contain translations which get added to the vector.
  EXPECT_THROW_DISCARD(makeTransformVar(sc_units::m) *
                           makeVectorVar(sc_units::mm),
                       except::UnitError);
}

TEST(VariableTest, mul_vector) {
  Eigen::Vector3d vec1(1, 2, 3);
  Eigen::Vector3d vec2(2, 4, 6);
  auto vec = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{1}, sc_units::m,
                                           Values{vec1});
  auto expected_vec = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{1},
                                                    sc_units::m, Values{vec2});
  auto scale =
      makeVariable<double>(Dims{}, Shape{1}, sc_units::one, Values{2.0});
  auto scale_with_variance = makeVariable<double>(
      Dims{}, Shape{1}, sc_units::one, Values{2.0}, Variances{1.0});

  auto left_scaled_vec = scale * vec;
  auto right_scaled_vec = vec * scale;

  EXPECT_THROW_DISCARD(vec * scale_with_variance, except::VariancesError);
  EXPECT_EQ(left_scaled_vec, expected_vec);
  EXPECT_EQ(right_scaled_vec, expected_vec);
}

TEST(VariableTest, divide_vector) {
  Eigen::Vector3d vec1(1, 2, 3);
  Eigen::Vector3d vec2(2, 4, 6);
  auto vec = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{1}, sc_units::m,
                                           Values{vec2});
  auto expected_vec = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{1},
                                                    sc_units::m, Values{vec1});
  auto scale =
      makeVariable<double>(Dims{}, Shape{1}, sc_units::one, Values{2.0});

  auto scaled_vec = vec / scale;

  EXPECT_EQ(scaled_vec, expected_vec);
}

TEST(Variable, 6d) {
  ASSERT_EQ(core::NDIM_OP_MAX, 6); // update this test if limit is increased
  auto a = makeVariable<double>(
      Dims{Dim("1"), Dim("2"), Dim("3"), Dim("4"), Dim("5"), Dim("6")},
      Shape{1, 2, 3, 4, 5, 6});
  auto b = makeVariable<double>(
      Dims{Dim("3"), Dim("2"), Dim("1"), Dim("4"), Dim("6"), Dim("5")},
      Shape{3, 2, 1, 4, 6, 5});
  copy(a, b);
  a += b;
}
