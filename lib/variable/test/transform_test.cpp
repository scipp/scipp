// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"
#include "test_print_variable.h"

#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

namespace {
const char *name = "transform_test";
}

TEST(TransformTest, Eigen_Vector3d_pass_by_value) {
  const auto var = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2},
      Values{Eigen::Vector3d{1.1, 2.2, 3.3}, Eigen::Vector3d{0.1, 0.2, 0.3}});
  const auto expected = makeVariable<Eigen::Vector3d>(
      Dims(), Shape(), Values{Eigen::Vector3d{1.0, 2.0, 3.0}});
  // Passing Eigen types by value often causes issues, ensure that it works.
  auto op = [](const auto x, const auto y) { return x - y; };

  const auto result = transform<pair_self_t<Eigen::Vector3d>>(
      var.slice({Dim::X, 0}), var.slice({Dim::X, 1}), op, name);

  EXPECT_EQ(result, expected);
}

TEST(TransformTest, mixed_precision) {
  auto d = makeVariable<double>(Values{1e-12});
  auto f = makeVariable<float>(Values{1e-12});
  auto base_d = makeVariable<double>(Values{1.0});
  auto base_f = makeVariable<float>(Values{1.0});
  auto op = [](const auto a, const auto b) { return a + b; };
  const auto sum_fd =
      transform<pair_custom_t<std::tuple<float, double>>>(base_f, d, op, name);
  const auto sum_dd =
      transform<pair_custom_t<std::tuple<double, double>>>(base_d, d, op, name);
  EXPECT_NE(sum_fd.values<double>()[0], 1.0f);
  EXPECT_EQ(sum_fd.values<double>()[0], 1.0f + 1e-12);
  EXPECT_NE(sum_dd.values<double>()[0], 1.0);
  EXPECT_EQ(sum_dd.values<double>()[0], 1.0 + 1e-12);
  const auto sum_ff =
      transform<pair_custom_t<std::tuple<float, float>>>(base_f, f, op, name);
  const auto sum_df =
      transform<pair_custom_t<std::tuple<double, float>>>(base_d, f, op, name);
  EXPECT_EQ(sum_ff.values<float>()[0], 1.0f);
  EXPECT_EQ(sum_ff.values<float>()[0], 1.0f + 1e-12f);
  EXPECT_NE(sum_df.values<double>()[0], 1.0);
  EXPECT_EQ(sum_df.values<double>()[0], 1.0 + 1e-12f);
}

TEST(TransformTest, mixed_precision_in_place) {
  auto d = makeVariable<double>(Values{1e-12});
  auto f = makeVariable<float>(Values{1e-12});
  auto sum_d = makeVariable<double>(Values{1.0});
  auto sum_f = makeVariable<float>(Values{1.0});
  auto op = [](auto &a, const auto b) { a += b; };
  transform_in_place<pair_custom_t<std::tuple<float, double>>>(sum_f, d, op,
                                                               name);
  transform_in_place<pair_custom_t<std::tuple<double, double>>>(sum_d, d, op,
                                                                name);
  EXPECT_EQ(sum_f.values<float>()[0], 1.0f);
  EXPECT_NE(sum_d.values<double>()[0], 1.0);
  EXPECT_EQ(sum_d.values<double>()[0], 1.0 + 1e-12);
  transform_in_place<pair_custom_t<std::tuple<float, float>>>(sum_f, f, op,
                                                              name);
  transform_in_place<pair_custom_t<std::tuple<double, float>>>(sum_d, f, op,
                                                               name);
  EXPECT_EQ(sum_f.values<float>()[0], 1.0f);
  EXPECT_NE(sum_d.values<double>()[0], 1.0 + 1e-12);
  EXPECT_EQ(sum_d.values<double>()[0], 1.0 + 1e-12 + 1e-12);
}

TEST(TransformTest, combined_uncertainty_propagation) {
  auto a = makeVariable<double>(Values{2.0}, Variances{0.1});
  auto a_2_step = copy(a);
  const auto b = makeVariable<double>(Values{3.0}, Variances{0.2});

  const auto abb = transform<pair_self_t<double>>(
      a, b, [](const auto &x, const auto &y) { return x * y + y; }, name);
  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &x, const auto y) { x = x * y + y; }, name);
  transform_in_place<pair_self_t<double>>(
      a_2_step, b, [](auto &x, const auto y) { x = x * y; }, name);
  transform_in_place<pair_self_t<double>>(
      a_2_step, b, [](auto &x, const auto y) { x = x + y; }, name);

  EXPECT_TRUE(equals(a.values<double>(), {2.0 * 3.0 + 3.0}));
  EXPECT_TRUE(equals(a.variances<double>(), {0.1 * 3 * 3 + 0.2 * 2 * 2 + 0.2}));
  EXPECT_EQ(abb, a);
  EXPECT_EQ(abb, a_2_step);
}

// It is possible to use transform with functors that call non-built-in
// functions. To do so we have to define that function for the ValueAndVariance
// helper.
constexpr auto user_op(const double) { return 123.0; }
constexpr auto user_op(const ValueAndVariance<double>) {
  return ValueAndVariance<double>{123.0, 456.0};
}
constexpr auto user_op(const sc_units::Unit &) { return sc_units::s; }

TEST(TransformTest, user_op_with_variances) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                  Values{1.1, 2.2}, Variances{1.1, 3.0});

  const auto result =
      transform<double>(var, [](auto x) { return user_op(x); }, name);
  transform_in_place<double>(var, [](auto &x) { x = user_op(x); }, name);

  auto expected = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::s,
                                       Values{123, 123}, Variances{456, 456});
  EXPECT_EQ(result, expected);
  EXPECT_EQ(result, var);
}

class TransformInPlaceDryRunTest : public ::testing::Test {
protected:
  static constexpr auto unary{[](auto &x) { x *= x; }};
  static constexpr auto binary{[](auto &x, const auto &y) { x *= y; }};
};

// Strictly speaking we should not have to test the failure cases --- even
// without dry-run, transform_in_place should not touch the data if there is a
// failure. Maybe this should be a parametrized test?
TEST_F(TransformInPlaceDryRunTest, unit_fail) {
  auto a = makeVariable<double>(Dims(), Shape(), sc_units::m);
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<double>(
                   a, [](auto &x) { x += x * x; }, name),
               std::runtime_error);
  EXPECT_EQ(a, original);
  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(
                   a, a * a, [](auto &x, const auto &y) { x += y; }, name),
               std::runtime_error);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, slice_unit_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m);
  const auto original = copy(a);

  EXPECT_THROW(
      dry_run::transform_in_place<double>(a.slice({Dim::X, 0}), unary, name),
      except::UnitError);
  EXPECT_EQ(a, original);
  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(
                   a.slice({Dim::X, 0}), a.slice({Dim::X, 0}), binary, name),
               except::UnitError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, dimensions_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m);
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m);
  const auto original = copy(a);

  EXPECT_THROW(
      dry_run::transform_in_place<pair_self_t<double>>(a, b, binary, name),
      except::DimensionError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, variances_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m);
  auto b = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m, Values{},
                                Variances{});
  const auto original = copy(a);

  EXPECT_THROW(
      dry_run::transform_in_place<pair_self_t<double>>(a, b, binary, name),
      except::VariancesError);
  EXPECT_EQ(a, original);
}

class TransformInPlaceBucketsDryRunTest : public TransformInPlaceDryRunTest {
protected:
  Variable indicesA = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 4}});
  Variable indicesB = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 3}, std::pair{3, 5}});
  Variable tableA =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, sc_units::m,
                           Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  Variable tableB =
      makeVariable<double>(Dims{Dim::Event}, Shape{5}, sc_units::m,
                           Values{1, 2, 3, 4, 5}, Variances{5, 6, 7, 8, 9});
  Variable a = make_bins(indicesA, Dim::Event, tableA);
  Variable b = make_bins(indicesB, Dim::Event, tableB);
};

TEST_F(TransformInPlaceBucketsDryRunTest, events_length_fail) {
  const auto original(a);
  EXPECT_THROW(
      dry_run::transform_in_place<pair_self_t<double>>(a, b, binary, name),
      except::BinnedDataError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceBucketsDryRunTest, variances_fail) {
  a = make_bins(indicesA, Dim::Event, values(tableA));
  const auto original(a);
  EXPECT_THROW(
      dry_run::transform_in_place<pair_self_t<double>>(a, b, binary, name),
      except::VariancesError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceBucketsDryRunTest, unchanged_if_success) {
  const auto original(a);
  dry_run::transform_in_place<double>(a, unary, name);
  EXPECT_EQ(a, original);
  dry_run::transform_in_place<pair_self_t<double>>(a, a, binary, name);
  EXPECT_EQ(a, original);
}

TEST(TransformFlagsTest, no_variance_on_arg) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto binary_op = [](auto x, auto y) { return x + y; };
  constexpr auto op_arg_0_has_flags =
      scipp::overloaded{transform_flags::expect_variance_arg<0>, binary_op};

  Variable out;
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_no_variance,
                                           op_arg_0_has_flags, name)));
  EXPECT_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_with_variance,
                                           op_arg_0_has_flags, name)),
      except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           op_arg_0_has_flags, name)));
  constexpr auto op_arg_1_has_flags =
      scipp::overloaded{transform_flags::expect_variance_arg<1>, binary_op};
  EXPECT_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_no_variance,
                                           op_arg_1_has_flags, name)),
      except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_with_variance,
                                           op_arg_1_has_flags, name)));
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           op_arg_1_has_flags, name)));
  constexpr auto all_args_with_flag =
      scipp::overloaded{transform_flags::expect_variance_arg<0>,
                        transform_flags::expect_variance_arg<1>, binary_op};
  EXPECT_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_no_variance,
                                           all_args_with_flag, name)),
      except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           all_args_with_flag, name)));
}

TEST(TransformFlagsTest, no_variance_on_arg_in_place) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto unary_in_place = [](auto &, auto) {};
  constexpr auto op_arg_0_has_flags = scipp::overloaded{
      transform_flags::expect_variance_arg<0>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_no_variance, op_arg_0_has_flags, name),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op_arg_0_has_flags, name));
  constexpr auto op_arg_1_has_flags = scipp::overloaded{
      transform_flags::expect_variance_arg<1>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_no_variance, op_arg_1_has_flags, name),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op_arg_1_has_flags, name));
}

TEST(TransformFlagsTest, variance_on_arg) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto binary_op = [](auto x, auto y) { return x + y; };
  constexpr auto op_arg_0_has_flags =
      scipp::overloaded{transform_flags::expect_no_variance_arg<0>, binary_op};
  Variable out;
  EXPECT_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_no_variance,
                                           op_arg_0_has_flags, name)),
      except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_with_variance,
                                           op_arg_0_has_flags, name)));
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_no_variance,
                                           op_arg_0_has_flags, name)));
  constexpr auto op_arg_1_has_flags =
      scipp::overloaded{transform_flags::expect_no_variance_arg<1>, binary_op};
  EXPECT_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_with_variance,
                                           op_arg_1_has_flags, name)),
      except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_no_variance,
                                           op_arg_1_has_flags, name)));
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_no_variance,
                                           op_arg_1_has_flags, name)));
  constexpr auto all_args_with_flag =
      scipp::overloaded{transform_flags::expect_no_variance_arg<0>,
                        transform_flags::expect_no_variance_arg<1>, binary_op};
  EXPECT_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           all_args_with_flag, name)),
      except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_no_variance,
                                           all_args_with_flag, name)));
}

TEST(TransformFlagsTest, no_out_variance) {
  constexpr auto op =
      overloaded{transform_flags::no_out_variance, element::arg_list<double>,
                 [](const auto) { return true; },
                 [](const sc_units::Unit &) { return sc_units::one; }};
  const auto var = makeVariable<double>(Values{1.0}, Variances{1.0});
  EXPECT_EQ(transform(var, op, name), true * sc_units::one);
}

TEST(TransformFlagsTest, variance_on_arg_in_place) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto unary_in_place = [](auto &, auto) {};
  constexpr auto op_arg_0_has_flags = scipp::overloaded{
      transform_flags::expect_no_variance_arg<0>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(var_with_variance,
                                                      var_with_variance,
                                                      op_arg_0_has_flags, name),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op_arg_0_has_flags, name));
  constexpr auto op_arg_1_has_flags = scipp::overloaded{
      transform_flags::expect_no_variance_arg<1>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(var_with_variance,
                                                      var_with_variance,
                                                      op_arg_1_has_flags, name),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op_arg_1_has_flags, name));
}

TEST(TransformFlagsTest, expect_in_variance_if_out_variance) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto inplace_op = [](auto &&x, const auto &y) { x += y; };
  constexpr auto op = overloaded{
      transform_flags::expect_in_variance_if_out_variance, inplace_op};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_with_variance, var_no_variance, op, name),
               except::VariancesError);
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_with_variance, op, name),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op, name));
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op, name));
}

TEST(TransformFlagsTest, expect_all_or_none_have_variance) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto binary_op = [](auto x, auto y) { return x + y; };
  constexpr auto op_has_flags = scipp::overloaded{
      transform_flags::expect_all_or_none_have_variance, binary_op};
  Variable out;
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_with_variance, var_no_variance, op_has_flags, name)),
               except::VariancesError);
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_no_variance, var_with_variance, op_has_flags, name)),
               except::VariancesError);
  EXPECT_NO_THROW((out = transform<std::tuple<double>>(
                       var_no_variance, var_no_variance, op_has_flags, name)));
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           op_has_flags, name)));
}

TEST(TransformFlagsTest, expect_all_or_none_have_variance_in_place) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto unary_op = [](auto &, auto) {};
  constexpr auto op_has_flags = scipp::overloaded{
      transform_flags::expect_all_or_none_have_variance, unary_op};
  Variable out;
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_with_variance, var_no_variance, op_has_flags, name),
               except::VariancesError);
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_with_variance, op_has_flags, name),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op_has_flags, name));
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op_has_flags, name));
}

TEST(TransformFlagsTest, expect_no_in_variance_if_out_cannot_have_variance) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto unary_op = [](const auto) { return false; };
  constexpr auto op_has_flags = scipp::overloaded{
      element::arg_list<double>,
      transform_flags::expect_no_in_variance_if_out_cannot_have_variance,
      unary_op, [](const sc_units::Unit &) { return sc_units::one; }};
  Variable out;
  EXPECT_THROW(out = transform(var_with_variance, op_has_flags, name),
               except::VariancesError);
  EXPECT_NO_THROW(out = transform(var_no_variance, op_has_flags, name));
}

class TransformBinElementsTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Variable var = make_bins(indices, Dim::X, copy(buffer));
};

TEST_F(TransformBinElementsTest, single_arg_in_place) {
  transform_in_place<double>(
      var,
      scipp::overloaded{transform_flags::expect_no_variance_arg<0>,
                        [](auto &x) { x *= x; }},
      name);
  Variable expected = make_bins(indices, Dim::X, buffer * buffer);
  EXPECT_EQ(var, expected);
}
