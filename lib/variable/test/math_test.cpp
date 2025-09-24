// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <scipp/variable/math.h>

#include <tuple>

#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/element/math.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/pow.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

template <typename T> class VariableMathTest : public ::testing::Test {};
using FloatTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(VariableMathTest, FloatTypes);

TYPED_TEST(VariableMathTest, abs) {
  for (TypeParam x : {0.0, -1.23, 3.45, -1.23456789}) {
    for (auto u : {sc_units::dimensionless, sc_units::m}) {
      const auto v = x * u;
      const auto ref = element::abs(x);
      EXPECT_EQ(abs(v), ref * u);
    }
  }
}

TEST(Variable, abs_out_arg) {
  const auto x = -1.23 * sc_units::m;
  auto out = 0.0 * sc_units::dimensionless;
  const auto &view = abs(x, out);

  EXPECT_EQ(x, -1.23 * sc_units::m);
  EXPECT_EQ(&view, &out);
  EXPECT_EQ(view, 1.23 * sc_units::m);
}

TEST(Variable, abs_out_arg_self) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{-1.23, 0.0});
  auto out = x.slice({Dim::X, 1});
  auto &view = abs(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(x, makeVariable<double>(Dims{Dim::X}, Shape{2},
                                    Values{-1.23, element::abs(-1.23)}));
  EXPECT_EQ(&view, &out);
}

TEST(Variable, norm_of_vector) {
  Eigen::Vector3d v1(1, 0, -1);
  Eigen::Vector3d v2(1, 1, 0);
  Eigen::Vector3d v3(0, 0, -2);
  auto reference = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, sc_units::m,
      Values{element::norm(v1), element::norm(v2), element::norm(v3)});
  auto var = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{v1, v2, v3});
  EXPECT_EQ(norm(var), reference);
}

TEST(Variable, pow_unit_exponent_dims) {
  const Variable base = 2.0 * sc_units::m;
  const Variable scalar_exponent = 3.0 * sc_units::one;
  const Variable array_exponent = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_NO_THROW_DISCARD(pow(base, scalar_exponent));
  EXPECT_THROW_DISCARD(pow(base, array_exponent), except::DimensionError);
}

TEST(Variable, pow_unit_float_exponent) {
  EXPECT_NO_THROW_DISCARD(pow(1.0 * sc_units::one, 2.2 * sc_units::one));
  EXPECT_THROW_DISCARD(pow(1.0 * sc_units::m, 2.2 * sc_units::one),
                       except::UnitError);
  EXPECT_THROW_DISCARD(pow(int64_t{1} * sc_units::m, 2.2 * sc_units::one),
                       except::UnitError);

  auto out = -1.0 * sc_units::one;
  EXPECT_NO_THROW_DISCARD(pow(1.0 * sc_units::one, 2.2 * sc_units::one, out));
  EXPECT_THROW_DISCARD(pow(1.0 * sc_units::m, 2.2 * sc_units::one, out),
                       except::UnitError);
}

template <typename T> class VariablePowTest : public ::testing::Test {};
using PowTypes =
    ::testing::Types<std::tuple<double, double>, std::tuple<double, float>,
                     std::tuple<double, int64_t>, std::tuple<int64_t, double>,
                     std::tuple<int64_t, int64_t>,
                     std::tuple<int64_t, int32_t>>;
TYPED_TEST_SUITE(VariablePowTest, PowTypes);

TYPED_TEST(VariablePowTest, pow_unit) {
  using B = std::tuple_element_t<0, TypeParam>;
  using E = std::tuple_element_t<1, TypeParam>;

  const auto base_one = static_cast<B>(1) * sc_units::one;
  const auto exp_one = static_cast<E>(1) * sc_units::one;
  const auto exp_two = static_cast<E>(2) * sc_units::one;
  const auto exp_three = static_cast<E>(3) * sc_units::one;
  const auto exp_four = static_cast<E>(4) * sc_units::one;

  const auto base_m = static_cast<B>(1) * sc_units::m;
  const auto exp_m = static_cast<E>(1) * sc_units::m;
  const auto base_s = static_cast<B>(1) * sc_units::s;
  const auto exp_s = static_cast<E>(1) * sc_units::s;

  EXPECT_EQ(pow(base_one, exp_one).unit(), sc_units::one);
  EXPECT_EQ(pow(base_m, exp_one).unit(), sc_units::m);
  EXPECT_EQ(pow(base_s, exp_one).unit(), sc_units::s);
  EXPECT_EQ(pow(base_m, exp_two).unit(), sc_units::m * sc_units::m);
  EXPECT_EQ(pow(base_s, exp_two).unit(), sc_units::s * sc_units::s);
  EXPECT_EQ(pow(base_m, exp_three).unit(),
            sc_units::m * sc_units::m * sc_units::m);
  EXPECT_EQ(pow(base_s, exp_four).unit(),
            sc_units::s * sc_units::s * sc_units::s * sc_units::s);
  EXPECT_THROW_DISCARD(pow(base_one, exp_m), except::UnitError);
  EXPECT_THROW_DISCARD(pow(base_one, exp_s), except::UnitError);
  EXPECT_THROW_DISCARD(pow(base_s, exp_m), except::UnitError);
}

TYPED_TEST(VariablePowTest, pow_unit_in_place) {
  using B = std::tuple_element_t<0, TypeParam>;
  using E = std::tuple_element_t<1, TypeParam>;
  using O = std::common_type_t<B, E>;

  auto out = static_cast<O>(-1) * sc_units::one;
  auto ret = pow(static_cast<B>(1) * sc_units::m,
                 static_cast<E>(2) * sc_units::one, out);
  EXPECT_EQ(out.unit(), sc_units::m * sc_units::m);
  EXPECT_EQ(ret.unit(), sc_units::m * sc_units::m);
}

TYPED_TEST(VariablePowTest, pow_dims) {
  using B = std::tuple_element_t<0, TypeParam>;
  using E = std::tuple_element_t<1, TypeParam>;

  Dimensions x{{Dim::X, 2}};
  Dimensions y{{Dim::Y, 3}};
  Dimensions xy{{Dim::X, 2}, {Dim::Y, 3}};

  for (auto &&base_unit : {sc_units::one, sc_units::m, sc_units::s}) {
    const auto base_scalar = makeVariable<B>(Dims{}, base_unit);
    const auto base_x = makeVariable<B>(x, base_unit);
    const auto base_y = makeVariable<B>(y, base_unit);
    const auto base_xy = makeVariable<B>(xy, base_unit);
    const auto exp_scalar = makeVariable<E>(Dims{});
    const auto exp_x = makeVariable<E>(x);
    const auto exp_y = makeVariable<E>(y);
    const auto exp_xy = makeVariable<E>(xy);

    EXPECT_EQ(pow(base_scalar, exp_scalar).dims().ndim(), 0);

    EXPECT_EQ(pow(base_x, exp_scalar).dims(), x);
    if (base_unit == sc_units::one) {
      EXPECT_EQ(pow(base_scalar, exp_x).dims(), x);
      EXPECT_EQ(pow(base_x, exp_x).dims(), x);
      EXPECT_EQ(pow(base_x, exp_y).dims(), xy);

      EXPECT_EQ(pow(base_xy, exp_x).dims(), xy);
      EXPECT_EQ(pow(base_xy, exp_y).dims(), xy);
      EXPECT_EQ(pow(base_x, exp_xy).dims(), xy);
      EXPECT_EQ(pow(base_y, exp_xy).dims(), xy);
    }

    EXPECT_THROW_DISCARD(
        pow(makeVariable<B>(Dims{Dim::X}, Shape{4}, base_unit), exp_x),
        except::DimensionError);
  }
}

TYPED_TEST(VariablePowTest, pow_dims_in_place) {
  using B = std::tuple_element_t<0, TypeParam>;
  using E = std::tuple_element_t<1, TypeParam>;
  using O = std::common_type_t<B, E>;
  Dimensions x{{Dim::X, 2}};
  for (auto &&base_unit : {sc_units::one, sc_units::m, sc_units::s}) {
    const auto base_scalar = makeVariable<B>(Dims{}, base_unit);
    const auto base_x = makeVariable<B>(x, base_unit);
    const auto exp_scalar = makeVariable<E>(Dims{});
    const auto exp_x = makeVariable<E>(x);
    auto out_scalar = makeVariable<O>(Dims{});
    auto out_x = makeVariable<O>(x);
    EXPECT_EQ(pow(base_scalar, exp_scalar, out_scalar).dims().ndim(), 0);
    EXPECT_THROW_DISCARD(pow(base_x, exp_scalar, out_scalar),
                         except::DimensionError);
    EXPECT_EQ(pow(base_x, exp_scalar, out_x).dims(), x);
    if (base_unit == sc_units::one) {
      EXPECT_THROW_DISCARD(pow(base_scalar, exp_x, out_scalar),
                           except::DimensionError);
      EXPECT_EQ(pow(base_scalar, exp_x, out_x).dims(), x);
    }
  }
}

namespace {
template <class B, class E> void pow_check_negative_exponent_allowed() {
  const Variable base = makeVariable<B>(Dims{}, Values{2});
  EXPECT_NO_THROW_DISCARD(pow(base, makeVariable<double>(Dims{}, Values{3})));
  EXPECT_NO_THROW_DISCARD(pow(base, makeVariable<double>(Dims{}, Values{-3})));

  for (auto values :
       {std::vector{-3, 4}, std::vector{-3, -4}, std::vector{3, -4}}) {
    EXPECT_NO_THROW_DISCARD(
        pow(base, makeVariable<E>(Dims{Dim::X}, Shape{2}, Values(values))));
  }
}
} // namespace

TEST(Variable, pow_negative_exponent) {
  // Negative powers are *not* allowed when both arguments are integers.
  const Variable int_base = makeVariable<int64_t>(Dims{}, Values{2});
  EXPECT_NO_THROW_DISCARD(
      pow(int_base, makeVariable<int64_t>(Dims{}, Values{3})));
  EXPECT_THROW_DISCARD(pow(int_base, makeVariable<int64_t>(Dims{}, Values{-3})),
                       std::invalid_argument);
  EXPECT_NO_THROW_DISCARD(pow(
      int_base, makeVariable<int64_t>(Dims{Dim::X}, Shape{2}, Values{3, 4})));
  for (auto &&values :
       {std::vector{-3, 4}, std::vector{-3, -4}, std::vector{3, -4}}) {
    EXPECT_THROW_DISCARD(
        pow(int_base,
            makeVariable<int64_t>(Dims{Dim::X}, Shape{2}, Values(values))),
        std::invalid_argument);
  }

  // Negative powers are allowed when floats are involved.
  pow_check_negative_exponent_allowed<int64_t, double>();
  pow_check_negative_exponent_allowed<double, double>();
  pow_check_negative_exponent_allowed<int64_t, double>();
}

TEST(Variable, pow_value) {
  for (auto &&base_unit : {sc_units::one, sc_units::m}) {
    EXPECT_NEAR(pow(3.0 * base_unit, 4.0 * sc_units::one).value<double>(), 81.0,
                1e-12);
    EXPECT_NEAR(
        pow(int64_t{3} * base_unit, 4.0 * sc_units::one).value<double>(), 81.0,
        1e-12);
    EXPECT_NEAR(
        pow(3.0 * base_unit, int64_t{4} * sc_units::one).value<double>(), 81.0,
        1e-12);
    EXPECT_EQ(pow(int64_t{3} * base_unit, int64_t{4} * sc_units::one)
                  .value<int64_t>(),
              int64_t{81});

    EXPECT_NEAR(pow(3.0 * base_unit, -4.0 * sc_units::one).value<double>(),
                1.0 / 81.0, 1e-12);
    EXPECT_NEAR(
        pow(int64_t{3} * base_unit, -4.0 * sc_units::one).value<double>(),
        1.0 / 81.0, 1e-12);
    EXPECT_NEAR(
        pow(3.0 * base_unit, int64_t{-4} * sc_units::one).value<double>(),
        1.0 / 81.0, 1e-12);
  }
}

TEST(Variable, pow_value_in_place) {
  auto base = 3.0 * sc_units::one;
  const auto exponent = 2.0 * sc_units::one;
  auto out = -1.0 * sc_units::one;
  auto ret = pow(base, exponent, out);
  EXPECT_NEAR(out.value<double>(), 9.0, 1e-15);
  EXPECT_TRUE(ret.is_same(out));
  ret = pow(base, exponent, base);
  EXPECT_NEAR(base.value<double>(), 9.0, 1e-15);
  EXPECT_TRUE(ret.is_same(base));
}

TEST(Variable, pow_value_and_variance) {
  const auto base = makeVariable<double>(Dims{}, Values{4.0}, Variances{2.0});
  const auto result = pow(base, int64_t{2} * sc_units::one);
  EXPECT_NEAR(result.value<double>(), 16.0, 1e-14);
  // pow.var = (2 * (base.val ^ 1)) ^ 2 * base.var
  EXPECT_NEAR(result.variance<double>(), 64.0 * base.variance<double>(), 1e-14);

  const auto exponent_with_variance =
      makeVariable<double>(Dims{}, Values{2.0}, Variances{2.0});
  EXPECT_THROW_DISCARD(pow(base, exponent_with_variance),
                       except::VariancesError);
}

TEST(Variable, pow_binned_variable) {
  const auto buffer = makeVariable<double>(
      Dims{Dim::Event}, Shape{5}, Values{1.0, 2.0, 3.0, 4.0, 5.0}, sc_units::m);
  const auto indices = makeVariable<index_pair>(
      Dims{Dim::X}, Shape{2}, Values{index_pair{0, 2}, index_pair{2, 5}});
  const auto base = make_bins(indices, Dim::Event, buffer);
  const auto result = pow(base, int64_t{2} * sc_units::one);

  const auto expected_buffer = makeVariable<double>(
      Dims{Dim::Event}, Shape{5}, Values{1.0, 4.0, 9.0, 16.0, 25.0},
      sc_units::m * sc_units::m);
  const auto expected = make_bins(indices, Dim::Event, expected_buffer);

  EXPECT_EQ(result, expected);
}

TEST(Variable, pow_binned_variable_exp) {
  const auto buffer = makeVariable<double>(
      Dims{Dim::Event}, Shape{5}, Values{1.0, 2.0, 3.0, 4.0, 5.0}, sc_units::m);
  const auto indices = makeVariable<index_pair>(
      Dims{Dim::X}, Shape{2}, Values{index_pair{0, 2}, index_pair{2, 5}});
  const auto exponent = make_bins(indices, Dim::Event, buffer);
  EXPECT_THROW_DISCARD(pow(int64_t{2} * sc_units::one, exponent),
                       std::invalid_argument);
}

TYPED_TEST(VariableMathTest, sqrt) {
  for (TypeParam x : {0.0, 1.23, 1.23456789, 3.45}) {
    for (auto [uin, uout] :
         {std::tuple{sc_units::dimensionless, sc_units::dimensionless},
          std::tuple{sc_units::m * sc_units::m, sc_units::m}}) {
      const auto v = x * uin;
      const auto ref = element::sqrt(x);
      EXPECT_EQ(sqrt(v), ref * uout);
    }
  }
}

TEST(Variable, sqrt_out_arg) {
  auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.23, 0.0});
  auto out = x.slice({Dim::X, 1});
  auto &view = sqrt(x.slice({Dim::X, 0}), out);

  EXPECT_EQ(x, makeVariable<double>(Dims{Dim::X}, Shape{2},
                                    Values{1.23, element::sqrt(1.23)}));
  EXPECT_EQ(&view, &out);
}

TEST(Variable, dot_of_vector) {
  Eigen::Vector3d v1(1.1, 2.2, 3.3);
  Eigen::Vector3d v2(-4.4, -5.5, -6.6);
  Eigen::Vector3d v3(0, 0, 0);
  auto reference = makeVariable<double>(
      Dims{Dim::X}, Shape{3}, sc_units::m * sc_units::m,
      Values{element::dot(v1, v1), element::dot(v2, v2), element::dot(v3, v3)});
  auto var = makeVariable<Eigen::Vector3d>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                           Values{v1, v2, v3});
  const auto result = dot(var, var);
  EXPECT_TRUE(
      all(isclose(result, reference, 1e-14 * sc_units::one,
                  makeVariable<double>(Values{0.0}, sc_units::m * sc_units::m)))
          .value<bool>());
}

TEST(Variable, cross_of_vector) {
  Eigen::Vector3d v1(1, 0, 0);
  Eigen::Vector3d v2(0, 1, 0);
  Eigen::Vector3d v3(0, 0, 1);

  auto reference = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3},
      sc_units::Unit(sc_units::m) * sc_units::Unit(sc_units::m),
      Values{element::cross(v1, v2), element::cross(v2, v1),
             element::cross(v2, v2)});
  auto var1 = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3}, sc_units::Unit(sc_units::m), Values{v1, v2, v2});
  auto var2 = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{3}, sc_units::Unit(sc_units::m), Values{v2, v1, v2});
  EXPECT_EQ(cross(var1, var2), reference);
}

TEST(Variable, reciprocal) {
  auto var1 = makeVariable<double>(Values{2});
  auto var2 = makeVariable<double>(Values{0.5});
  ASSERT_EQ(reciprocal(var1), var2);
  var1 = makeVariable<double>(Values{2}, Variances{1});
  var2 = makeVariable<double>(Values{0.5}, Variances{0.0625});
  ASSERT_EQ(reciprocal(var1), var2);
}

TEST(Variable, reciprocal_out_arg_full_in_place) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                  Values{1, 4, 9});
  auto &view = reciprocal(var, var);
  EXPECT_EQ(var,
            makeVariable<double>(Dims{Dim::X}, Shape{3},
                                 sc_units::Unit(sc_units::one / sc_units::m),
                                 Values{1., 1. / 4., 1. / 9.}));
  EXPECT_EQ(&view, &var);
}

TEST(Variable, reciprocal_out_arg_partial) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, sc_units::m,
                                        Values{1, 4, 9});
  auto out = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m);
  auto &view = reciprocal(var.slice({Dim::X, 1, 3}), out);
  EXPECT_EQ(out,
            makeVariable<double>(Dims{Dim::X}, Shape{2},
                                 sc_units::Unit(sc_units::one / sc_units::m),
                                 Values{1. / 4., 1. / 9.}));
  EXPECT_EQ(&view, &out);
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
  const auto &view = exp(x, out);

  EXPECT_EQ(
      out, makeVariable<double>(dims, shape,
                                Values{element::exp(1.23), element::exp(0.0)}));
  EXPECT_EQ(&view, &out);
}

TEST(Variable, exp_bad_unit) {
  EXPECT_THROW_DISCARD(exp(0.0 * sc_units::s), except::UnitError);
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
  const auto &view = log(x, out);

  EXPECT_EQ(out,
            makeVariable<double>(
                dims, shape, Values{element::log(1.23), element::log(3.21)}));
  EXPECT_EQ(&view, &out);
}

TEST(Variable, log_bad_unit) {
  EXPECT_THROW_DISCARD(log(1.0 * sc_units::s), except::UnitError);
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
  const auto &view = log10(x, out);

  EXPECT_EQ(out, makeVariable<double>(
                     dims, shape,
                     Values{element::log10(1.23), element::log10(3.21)}));
  EXPECT_EQ(&view, &out);
}

TEST(Variable, log10_bad_unit) {
  EXPECT_THROW_DISCARD(log10(1.0 * sc_units::s), except::UnitError);
}

TEST(Variable, rint) {
  auto preRoundedVar = makeVariable<double>(
      Dims{scipp::sc_units::Dim::X}, Values{1.2, 2.9, 1.5, 2.5}, Shape{4});
  auto roundedVar = makeVariable<double>(Dims{scipp::sc_units::Dim::X},
                                         Values{1, 3, 2, 2}, Shape{4});
  EXPECT_EQ(rint(preRoundedVar), roundedVar);
}

TEST(Variable, ceil) {
  auto preRoundedVar = makeVariable<double>(
      Dims{scipp::sc_units::Dim::X}, Values{1.2, 2.9, 1.5, 2.5}, Shape{4});
  auto roundedVar = makeVariable<double>(Dims{scipp::sc_units::Dim::X},
                                         Values{2, 3, 2, 3}, Shape{4});
  EXPECT_EQ(ceil(preRoundedVar), roundedVar);
}

TEST(Variable, floor) {
  auto preRoundedVar = makeVariable<double>(
      Dims{scipp::sc_units::Dim::X}, Values{1.2, 2.9, 1.5, 2.5}, Shape{4});
  auto roundedVar = makeVariable<double>(Dims{scipp::sc_units::Dim::X},
                                         Values{1, 2, 1, 2}, Shape{4});
  EXPECT_EQ(floor(preRoundedVar), roundedVar);
}

TEST(Variable, midpoints_throws_with_scalar_input) {
  EXPECT_THROW_DISCARD(
      midpoints(makeVariable<int64_t>(Dims{}, Shape{}, Values{2})),
      except::DimensionError);
  EXPECT_THROW_DISCARD(
      midpoints(makeVariable<int64_t>(Dims{}, Shape{}, Values{2}), Dim::X),
      except::DimensionError);
}

TEST(Variable, midpoints_1d_throws_with_single_element) {
  EXPECT_THROW_DISCARD(
      midpoints(makeVariable<int64_t>(Dims{Dim::X}, Shape{1}, Values{1})),
      except::DimensionError);
  EXPECT_THROW_DISCARD(
      midpoints(makeVariable<int64_t>(Dims{Dim::X}, Shape{1}, Values{1}),
                Dim::X),
      except::DimensionError);
}

TEST(Variable, midpoints_1d_2_elements) {
  const auto var = makeVariable<int64_t>(Dims{Dim::X}, Shape{2}, Values{3, 7});
  const auto expected = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{5});
  EXPECT_EQ(midpoints(var), expected);
}

TEST(Variable, midpoints_1d_many_elements) {
  const auto var = makeVariable<int64_t>(Dims{Dim::X}, Shape{7},
                                         Values{-3, -1, 0, 1, 1, 3, 6});
  const auto expected = makeVariable<double>(
      Dims{Dim::X}, Shape{6}, Values{-2., -0.5, 0.5, 1., 2., 4.5});
  EXPECT_EQ(midpoints(var), expected);
}

TEST(Variable, midpoints_2d_requires_dim_argument) {
  const auto var =
      makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{1, 1}, Values{3});
  EXPECT_THROW_DISCARD(midpoints(var), std::invalid_argument);
}

TEST(Variable, midpoints_2d_many_elements_inner) {
  const auto var = makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                         Values{5, 1, -2, 3, 1, 1});
  const auto expected = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                             Values{3., -0.5, 2., 1.});
  EXPECT_EQ(midpoints(var, Dim::Y), expected);
}

TEST(Variable, midpoints_2d_2_elements_outer) {
  const auto var = makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                         Values{5, 1, -2, 3, 1, 1});
  const auto expected = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{1, 3},
                                             Values{4., 1., -0.5});
  EXPECT_EQ(midpoints(var, Dim::X), expected);
}
