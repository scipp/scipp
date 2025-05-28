// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include <units/units.hpp>

#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"

using namespace scipp;

class Variable_comparison_operators : public ::testing::Test {
private:
  template <class A, class B>
  void expect_eq_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
    EXPECT_TRUE(equals_nan(a, b));
    EXPECT_TRUE(equals_nan(b, a));
  }
  template <class A, class B>
  void expect_ne_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
    EXPECT_FALSE(equals_nan(a, b));
    EXPECT_FALSE(equals_nan(b, a));
  }

protected:
  void expect_eq(const Variable &a, const Variable &b) const {
    expect_eq_impl(a, Variable(b));
    expect_eq_impl(Variable(a), b);
    expect_eq_impl(Variable(a), Variable(b));
    expect_eq_impl(a, copy(b));
    expect_eq_impl(copy(a), b);
    expect_eq_impl(copy(a), copy(b));
  }
  void expect_ne(const Variable &a, const Variable &b) const {
    expect_ne_impl(a, Variable(b));
    expect_ne_impl(Variable(a), b);
    expect_ne_impl(Variable(a), Variable(b));
    expect_ne_impl(a, copy(b));
    expect_ne_impl(copy(a), b);
    expect_ne_impl(copy(a), copy(b));
  }
};

TEST_F(Variable_comparison_operators, values_0d) {
  const auto base = makeVariable<double>(Values{1.1});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(Values{1.1}));
  expect_ne(base, makeVariable<double>(Values{1.2}));
}

TEST_F(Variable_comparison_operators, values_1d) {
  const auto base =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  expect_eq(base, base);
  expect_eq(base,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2}));
  expect_ne(base,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.3}));
}

TEST_F(Variable_comparison_operators, values_2d) {
  const auto base =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1}, Values{1.1, 2.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                       Values{1.1, 2.2}));
  expect_ne(base, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                       Values{1.1, 2.3}));
}

TEST_F(Variable_comparison_operators, variances_0d) {
  const auto base = makeVariable<double>(Values{1.1}, Variances{0.1});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(Values{1.1}, Variances{0.1}));
  expect_ne(base, makeVariable<double>(Values{1.1}));
  expect_ne(base, makeVariable<double>(Values{1.1}, Variances{0.2}));
}

TEST_F(Variable_comparison_operators, variances_1d) {
  const auto base = makeVariable<double>(Dims{Dim::X}, Shape{2},
                                         Values{1.1, 2.2}, Variances{0.1, 0.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                       Variances{0.1, 0.2}));
  expect_ne(base,
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2}));
  expect_ne(base, makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                       Variances{0.1, 0.3}));
}

TEST_F(Variable_comparison_operators, variances_2d) {
  const auto base = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                         Values{1.1, 2.2}, Variances{0.1, 0.2});
  expect_eq(base, base);
  expect_eq(base, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                       Values{1.1, 2.2}, Variances{0.1, 0.2}));
  expect_ne(base, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                       Values{1.1, 2.2}));
  expect_ne(base, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 1},
                                       Values{1.1, 2.2}, Variances{0.1, 0.3}));
}

TEST_F(Variable_comparison_operators, dimension_mismatch) {
  expect_ne(makeVariable<double>(Values{1.1}),
            makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.1}));
  expect_ne(makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.1}),
            makeVariable<double>(Dims{Dim::Y}, Shape{1}, Values{1.1}));
}

TEST_F(Variable_comparison_operators, dimension_transpose) {
  expect_ne(
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{1, 1}, Values{1.1}),
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 1}, Values{1.1}));
}

TEST_F(Variable_comparison_operators, dimension_length) {
  expect_ne(makeVariable<double>(Dims{Dim::X}, Shape{1}),
            makeVariable<double>(Dims{Dim::X}, Shape{2}));
}

TEST_F(Variable_comparison_operators, unit) {
  const auto m =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, sc_units::m, Values{1.1});
  const auto s =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, sc_units::s, Values{1.1});
  expect_eq(m, m);
  expect_ne(m, s);
}

TEST_F(Variable_comparison_operators, dtype) {
  const auto base = makeVariable<double>(Values{1.0});
  expect_ne(base, makeVariable<float>(Values{1.0}));
}

TEST_F(Variable_comparison_operators, dense_events) {
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  auto buf = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  auto events = make_bins(indices, Dim::X, buf);
  auto dense = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2l, 0l});
  expect_ne(dense, events);
}

TEST_F(Variable_comparison_operators, events) {
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable indices2 = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 3}, std::pair{3, 4}});
  auto buf = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  auto buf_with_vars = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                            Values{1, 2, 3, 4}, Variances{});
  auto a = make_bins(indices, Dim::X, buf);
  auto b = make_bins(indices, Dim::X, buf);
  auto c = make_bins(indices, Dim::X, buf * (2.0 * sc_units::one));
  auto d = make_bins(indices2, Dim::X, buf);
  auto a_with_vars = make_bins(indices, Dim::X, buf_with_vars);

  expect_eq(a, a);
  expect_eq(a, b);
  expect_ne(a, c);
  expect_ne(a, d);
  expect_ne(a, a_with_vars);
}

TEST_F(Variable_comparison_operators, slice) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                       sc_units::m, Values{1, 2, 3, 4, 5, 6},
                                       Variances{7, 8, 9, 10, 11, 12});
  const auto sliced = xy.slice({Dim::X, 1, 2}).slice({Dim::Y, 1, 3});
  const auto section =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{1, 2}, sc_units::m,
                           Values{5, 6}, Variances{11, 12});
  EXPECT_FALSE(equals(sliced.strides(), section.strides()));
  EXPECT_NE(sliced.offset(), section.offset());
  expect_eq(sliced, section);
}

TEST_F(Variable_comparison_operators, broadcast) {
  const auto a = makeVariable<double>(Dimensions(Dim::X, 3), sc_units::m,
                                      Values{1.2, 1.2, 1.2});
  const auto b = broadcast(1.2 * sc_units::m, Dimensions(Dim::X, 3));
  EXPECT_FALSE(equals(a.strides(), b.strides()));
  EXPECT_TRUE(equals(b.strides(), {0}));
  expect_eq(a, b);
}

TEST_F(Variable_comparison_operators, transpose) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                       sc_units::m, Values{1, 2, 3, 4});
  const auto yx = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       sc_units::m, Values{1, 3, 2, 4});
  expect_ne(xy, yx);
  const auto transposed = transpose(yx);
  EXPECT_FALSE(equals(xy.strides(), transposed.strides()));
  EXPECT_TRUE(equals(transposed.strides(), {1, 2}));
  expect_eq(xy, transposed);
}

TEST_F(Variable_comparison_operators, readonly) {
  const auto var = makeVariable<double>(Values{1.0});
  const auto readonly = var.as_const();
  EXPECT_FALSE(var.is_readonly());
  EXPECT_TRUE(readonly.is_readonly());
  expect_eq(var, readonly);
}

TEST_F(Variable_comparison_operators, aligned) {
  const auto var = makeVariable<double>(Values{1.0});
  auto unaligned = var;
  unaligned.set_aligned(false);
  EXPECT_TRUE(var.is_aligned());
  EXPECT_FALSE(unaligned.is_aligned());
  expect_eq(var, unaligned);
}
