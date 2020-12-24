// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <scipp/variable/math.h>

#include <tuple>

#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/element/math.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

template <typename T> class VariableMathTest : public ::testing::Test {};
using FloatTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(VariableMathTest, FloatTypes);

TYPED_TEST(VariableMathTest, abs) {
  for (TypeParam x : {0.0, -1.23, 3.45, -1.23456789}) {
    for (auto u : {units::dimensionless, units::m}) {
      const auto v = x * u;
      const auto ref = element::abs(x);
      EXPECT_EQ(abs(v), ref * u);
    }
  }
}

TEST(Variable, abs_move) {
  auto var = makeVariable<double>(Values{-1.23});
  const auto ptr = var.values<double>().data();
  auto out = abs(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{element::abs(-1.23)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, abs_out_arg) {
  const auto x = -1.23 * units::m;
  auto out = 0.0 * units::dimensionless;
  const auto view = abs(x, out);

  EXPECT_EQ(x, -1.23 * units::m);
  EXPECT_EQ(view, out);
  EXPECT_EQ(view, 1.23 * units::m);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, abs_out_arg_self) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{-1.23, 0.0});
  auto out = x.slice({Dim::X, 1});
  auto view = abs(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(x, makeVariable<double>(Dims{Dim::X}, Shape{2},
                                    Values{-1.23, element::abs(-1.23)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), x);
}

TEST(Variable, norm_of_vector) {
  Eigen::Vector3d v1(1, 0, -1);
  Eigen::Vector3d v2(1, 1, 0);
  Eigen::Vector3d v3(0, 0, -2);
  auto reference = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, units::m,
      Values{element::norm(v1), element::norm(v2), element::norm(v3)});
  auto var = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{3}, units::m,
                                           Values{v1, v2, v3});
  EXPECT_EQ(norm(var), reference);
}

TYPED_TEST(VariableMathTest, sqrt) {
  for (TypeParam x : {0.0, 1.23, 1.23456789, 3.45}) {
    for (auto [uin, uout] :
         {std::tuple{units::dimensionless, units::dimensionless},
          std::tuple{units::m * units::m, units::m}}) {
      const auto v = x * uin;
      const auto ref = element::sqrt(x);
      EXPECT_EQ(sqrt(v), ref * uout);
    }
  }
}

TEST(Variable, sqrt_move) {
  auto var = makeVariable<double>(Values{1.23});
  const auto ptr = var.values<double>().data();
  auto out = sqrt(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{element::sqrt(1.23)}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, sqrt_out_arg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.23, 0.0});
  auto out = x.slice({Dim::X, 1});
  auto view = sqrt(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(x, makeVariable<double>(Dims{Dim::X}, Shape{2},
                                    Values{1.23, element::sqrt(1.23)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), x);
}

TEST(Variable, dot_of_vector) {
  Eigen::Vector3d v1(1.1, 2.2, 3.3);
  Eigen::Vector3d v2(-4.4, -5.5, -6.6);
  Eigen::Vector3d v3(0, 0, 0);
  auto reference = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, units::Unit(units::m) * units::Unit(units::m),
      Values{element::dot(v1, v1), element::dot(v2, v2), element::dot(v3, v3)});
  auto var = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3}, units::Unit(units::m), Values{v1, v2, v3});
  EXPECT_EQ(dot(var, var), reference);
}

TEST(Variable, reciprocal) {
  auto var1 = makeVariable<double>(Values{2});
  auto var2 = makeVariable<double>(Values{0.5});
  ASSERT_EQ(reciprocal(var1), var2);
  var1 = makeVariable<double>(Values{2}, Variances{1});
  var2 = makeVariable<double>(Values{0.5}, Variances{0.0625});
  ASSERT_EQ(reciprocal(var1), var2);
}

TEST(Variable, reciprocal_move) {
  auto var = makeVariable<double>(Values{4});
  const auto ptr = var.values<double>().data();
  auto out = reciprocal(std::move(var));
  EXPECT_EQ(out, makeVariable<double>(Values{0.25}));
  EXPECT_EQ(out.values<double>().data(), ptr);
}

TEST(Variable, reciprocal_out_arg_full_in_place) {
  auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m, Values{1, 4, 9});
  auto view = reciprocal(var, var);
  EXPECT_EQ(var, makeVariable<double>(Dims{Dim::X}, Shape{3},
                                      units::Unit(units::one / units::m),
                                      Values{1., 1. / 4., 1. / 9.}));
  EXPECT_EQ(view, var);
  EXPECT_EQ(view.underlying(), var);
}

TEST(Variable, reciprocal_out_arg_partial) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, units::m, Values{1, 4, 9});
  auto out = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  auto view = reciprocal(var.slice({Dim::X, 1, 3}), out);
  EXPECT_EQ(out, makeVariable<double>(Dims{Dim::X}, Shape{2},
                                      units::Unit(units::one / units::m),
                                      Values{1. / 4., 1. / 9.}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TYPED_TEST(VariableMathTest, exp) {
  for (TypeParam x : {0.0, -1.23, 3.45, -1.23456789}) {
    const auto v = makeVariable<TypeParam>(Values{x});
    const auto ref = element::exp(x);
    EXPECT_EQ(exp(v), makeVariable<TypeParam>(Values{ref}));
  }
}

TEST(Variable, exp_out_arg) {
  Dims dims{Dim::X};
  Shape shape{2};
  const auto x = makeVariable<double>(dims, shape, Values{1.23, 0.0});
  auto out = makeVariable<double>(dims, shape, Values{0.0, 0.0});
  const auto view = exp(x, out);

  EXPECT_EQ(
      out, makeVariable<double>(dims, shape,
                                Values{element::exp(1.23), element::exp(0.0)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, exp_bad_unit) {
  EXPECT_THROW(static_cast<void>(exp(0.0 * units::s)), except::UnitError);
}

TYPED_TEST(VariableMathTest, log) {
  for (TypeParam x : {0.1, 1.23, 3.45}) {
    const auto v = makeVariable<TypeParam>(Values{x});
    const auto ref = element::log(x);
    EXPECT_EQ(log(v), makeVariable<TypeParam>(Values{ref}));
  }
}

TEST(Variable, log_out_arg) {
  Dims dims{Dim::X};
  Shape shape{2};
  const auto x = makeVariable<double>(dims, shape, Values{1.23, 3.21});
  auto out = makeVariable<double>(dims, shape, Values{0.0, 0.0});
  const auto view = log(x, out);

  EXPECT_EQ(out,
            makeVariable<double>(
                dims, shape, Values{element::log(1.23), element::log(3.21)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, log_bad_unit) {
  EXPECT_THROW(static_cast<void>(log(1.0 * units::s)), except::UnitError);
}

TYPED_TEST(VariableMathTest, log10) {
  for (TypeParam x : {0.1, 1.23, 3.45}) {
    const auto v = makeVariable<TypeParam>(Values{x});
    const auto ref = element::log10(x);
    EXPECT_EQ(log10(v), makeVariable<TypeParam>(Values{ref}));
  }
}

TEST(Variable, log10_out_arg) {
  Dims dims{Dim::X};
  Shape shape{2};
  const auto x = makeVariable<double>(dims, shape, Values{1.23, 3.21});
  auto out = makeVariable<double>(dims, shape, Values{0.0, 0.0});
  const auto view = log10(x, out);

  EXPECT_EQ(out, makeVariable<double>(
                     dims, shape,
                     Values{element::log10(1.23), element::log10(3.21)}));
  EXPECT_EQ(view, out);
  EXPECT_EQ(view.underlying(), out);
}

TEST(Variable, log10_bad_unit) {
  EXPECT_THROW(static_cast<void>(log10(1.0 * units::s)), except::UnitError);
}

TYPED_TEST(VariableMathTest, floor_div_test_values) {
  auto a = makeVariable<TypeParam>(Values{1}, units::m);
  auto b = makeVariable<TypeParam>(Values{2}, units::m);
  EXPECT_EQ(floor_div(a, b),
            makeVariable<TypeParam>(Values{0}, units::dimensionless));
  EXPECT_EQ(floor_div(a, a),
            makeVariable<TypeParam>(Values{1}, units::dimensionless));
}
