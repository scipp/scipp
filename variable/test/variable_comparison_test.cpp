// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
  }
  template <class A, class B>
  void expect_ne_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
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
      makeVariable<double>(Dims{Dim::X}, Shape{1}, units::m, Values{1.1});
  const auto s =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, units::s, Values{1.1});
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
  auto c = make_bins(indices, Dim::X, buf * (2.0 * units::one));
  auto d = make_bins(indices2, Dim::X, buf);
  auto a_with_vars = make_bins(indices, Dim::X, buf_with_vars);

  expect_eq(a, a);
  expect_eq(a, b);
  expect_ne(a, c);
  expect_ne(a, d);
  expect_ne(a, a_with_vars);
}
