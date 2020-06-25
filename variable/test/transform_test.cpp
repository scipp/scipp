// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "make_events.h"
#include "test_macros.h"

#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

class TransformUnaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{
      overloaded{[](auto &x) { x *= 2.0; }, [](units::Unit &) {}}};
  static constexpr auto op{
      overloaded{[](const auto x) { return x * 2.0; },
                 [](const units::Unit &unit) { return unit; }}};
};

TEST_F(TransformUnaryTest, dense) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(var.values<double>(), {1.1 * 2.0, 2.2 * 2.0}));
  // In-place transform used to check result of non-in-place transform.
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, dense_with_variances) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                  Variances{1.1, 3.0});

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(var.values<double>(), {2.2, 4.4}));
  EXPECT_TRUE(equals(var.variances<double>(), {4.4, 12.0}));
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, elements_of_events) {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto vals = var.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(vals[0], {1 * 2.0, 2 * 2.0, 3 * 2.0}));
  EXPECT_TRUE(equals(vals[1], {4 * 2.0}));
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, elements_of_events_with_variance) {
  auto var = makeVariable<event_list<double>>(Dimensions{Dim::Y, 2}, Values{},
                                              Variances{});
  auto vals = var.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = var.variances<event_list<double>>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {4.4};

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(vals[0], {2, 4, 6}));
  EXPECT_TRUE(equals(vals[1], {8}));
  EXPECT_TRUE(equals(vars[0], {4.4, 8.8, 13.2}));
  EXPECT_TRUE(equals(vars[1], {17.6}));
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, events_values_variances_size_fail) {
  Dimensions dims(Dim::Y, 2);
  auto a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(2), event_list<double>(1)},
      Variances{event_list<double>(2), event_list<double>(2)});

  ASSERT_THROW_NODISCARD(transform<double>(a, op), except::SizeError);
  ASSERT_THROW(transform_in_place<double>(a, op_in_place), except::SizeError);
  a.variances<event_list<double>>()[1].resize(1);
  ASSERT_NO_THROW_NODISCARD(transform<double>(a, op));
  ASSERT_NO_THROW(transform_in_place<double>(a, op_in_place));
}

TEST_F(TransformUnaryTest, in_place_unit_change) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1.0, 2.0});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2},
                           units::Unit(units::m * units::m), Values{1.0, 4.0});
  auto op_ = [](auto &&a) { a *= a; };
  Variable result;

  result = var;
  transform_in_place<double>(result, op_);
  EXPECT_EQ(result, expected);

  // Unit changes but we are transforming only parts of data -> not possible.
  result = var;
  EXPECT_THROW(transform_in_place<double>(result.slice({Dim::X, 1}), op_),
               except::UnitError);
}

TEST(TransformTest, apply_unary_implicit_conversion) {
  const auto var =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out = transform<float>(
      var, overloaded{[](const auto x) { return -1.0 * x; },
                      [](const units::Unit &unit) { return unit; }});
  EXPECT_TRUE(equals(out.values<double>(), {-1.1f, -2.2f}));
}

TEST(TransformTest, apply_unary_dtype_preserved) {
  const auto varD =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto varF =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  auto outD = transform<double, float>(varD, [](const auto x) { return -x; });
  auto outF = transform<double, float>(varF, [](const auto x) { return -x; });
  EXPECT_TRUE(equals(outD.values<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.values<float>(), {-1.1f, -2.2f}));
}

TEST(TransformTest, dtype_bool) {
  auto var = makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});

  EXPECT_EQ(
      transform<bool>(var, overloaded{[](const units::Unit &u) { return u; },
                                      [](const auto x) { return !x; }}),
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));

  EXPECT_EQ(transform<pair_self_t<bool>>(
                var, var,
                overloaded{
                    [](const units::Unit &a, const units::Unit &) { return a; },
                    [](const auto x, const auto y) { return !x || y; }}),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));

  transform_in_place<bool>(
      var, overloaded{[](units::Unit &) {}, [](auto &x) { x = !x; }});
  EXPECT_EQ(var,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));

  transform_in_place<pair_self_t<bool>>(
      var, var,
      overloaded{[](units::Unit &, const units::Unit &) {},
                 [](auto &x, const auto &y) { x = !x || y; }});
  EXPECT_EQ(var,
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, true}));
}

class TransformBinaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{[](auto &x, const auto &y) { x *= y; }};
  static constexpr auto op{[](const auto &x, const auto &y) { return x * y; }};
};

TEST_F(TransformBinaryTest, dense) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<double>(Values{3.3});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  const auto ba = transform<pair_self_t<double>>(b, a, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3, 2.2 * 3.3}));
  EXPECT_EQ(ab, ba);
  EXPECT_EQ(ab, a);
  EXPECT_EQ(ba, a);
}

TEST_F(TransformBinaryTest, dims_and_shape_fail_in_place) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2});
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2});
  auto c = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});

  EXPECT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place));
  EXPECT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, c, op_in_place));
}

TEST_F(TransformBinaryTest, dims_and_shape_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{4});
  auto b = makeVariable<double>(Dims{Dim::X}, Shape{2});
  auto c = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2});

  EXPECT_ANY_THROW(auto v = transform<pair_self_t<double>>(a, b, op));
  EXPECT_ANY_THROW(auto v = transform<pair_self_t<double>>(a, c, op));
}

TEST_F(TransformBinaryTest, dense_mixed_type) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<float>(Values{3.3});

  const auto ab = transform<pair_custom_t<std::tuple<double, float>>>(a, b, op);
  const auto ba = transform<pair_custom_t<std::tuple<float, double>>>(b, a, op);
  transform_in_place<pair_custom_t<std::tuple<double, float>>>(a, b,
                                                               op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3f, 2.2 * 3.3f}));
  EXPECT_EQ(ab, ba);
  EXPECT_EQ(ab, a);
  EXPECT_EQ(ba, a);
}

TEST_F(TransformBinaryTest, var_with_view) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0.1, 3.3});

  auto ab = transform<pair_self_t<double>>(a, b.slice({Dim::Y, 1}), op);
  transform_in_place<pair_self_t<double>>(a, b.slice({Dim::Y, 1}), op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3, 2.2 * 3.3}));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_without_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  Variable slice_copy(a.slice({Dim::X, 1}));
  auto reference = a * slice_copy;
  transform_in_place<pair_self_t<double>>(a, a.slice({Dim::X, 1}), op_in_place);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_with_variance) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2},
                                Variances{1.0, 2.0});
  Variable slice_copy(a.slice({Dim::X, 1}));
  auto reference = a * slice_copy;
  // With self-overlap the implementation needs to make a copy of the rhs. This
  // is a regression test: An initial implementation was unintentionally
  // dropping the variances when making that copy.
  transform_in_place<pair_self_t<double>>(a, a.slice({Dim::X, 1}), op_in_place);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, view_with_var) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<double>(Values{3.3});

  transform_in_place<pair_self_t<double>>(a.slice({Dim::X, 1}), b, op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1, 2.2 * 3.3}));
}

TEST_F(TransformBinaryTest, view_with_view) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.1, 2.2});
  const auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{0.1, 3.3});

  transform_in_place<pair_self_t<double>>(a.slice({Dim::X, 1}),
                                          b.slice({Dim::Y, 1}), op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1, 2.2 * 3.3}));
}

TEST_F(TransformBinaryTest, dense_events) {
  auto events = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto events_ = events.values<event_list<double>>();
  events_[0] = {1, 2, 3};
  events_[1] = {4};
  auto dense = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.5, 0.5});

  const auto ab = transform<pair_self_t<double>>(events, dense, op);
  const auto ba = transform<pair_self_t<double>>(dense, events, op);
  transform_in_place<pair_self_t<double>>(events, dense, op_in_place);

  EXPECT_TRUE(equals(events_[0], {1.5, 3.0, 4.5}));
  EXPECT_TRUE(equals(events_[1], {2.0}));
  EXPECT_EQ(ab, events);
  EXPECT_EQ(ba, events);
}

TEST_F(TransformBinaryTest, events_size_fail) {
  Dimensions dims(Dim::Y, 2);
  auto a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(2), event_list<double>(1)});
  auto b = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(2), event_list<double>()});

  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, b, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place),
               except::SizeError);
  b.values<event_list<double>>()[1].resize(1);
  ASSERT_NO_THROW_NODISCARD(transform<pair_self_t<double>>(a, b, op));
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place));
  b.values<event_list<double>>()[1].resize(2);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, b, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place),
               except::SizeError);
}

TEST_F(TransformBinaryTest, in_place_unit_change) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1.0, 2.0});
  const auto expected =
      makeVariable<double>(Dims{Dim::X}, Shape{2},
                           units::Unit(units::m * units::m), Values{1.0, 4.0});
  auto op_ = [](auto &&a, auto &&b) { a *= b; };
  Variable result;

  result = var;
  transform_in_place<pair_self_t<double>>(result, var, op_);
  EXPECT_EQ(result, expected);

  // Unit changes but we are transforming only parts of data -> not possible.
  result = var;
  EXPECT_THROW(transform_in_place<pair_self_t<double>>(
                   result.slice({Dim::X, 1}), var.slice({Dim::X, 1}), op_),
               except::UnitError);
}

TEST(AccumulateTest, in_place) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1.0, 2.0});
  const auto expected = makeVariable<double>(Values{3.0});
  auto op_ = [](auto &&a, auto &&b) { a += b; };
  Variable result;

  // Note how accumulate is ignoring the unit.
  result = makeVariable<double>(Values{double{}});
  accumulate_in_place<pair_self_t<double>>(result, var, op_);
  EXPECT_EQ(result, expected);
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
      var.slice({Dim::X, 0}), var.slice({Dim::X, 1}), op);

  EXPECT_EQ(result, expected);
}

TEST(TransformTest, mixed_precision) {
  auto d = makeVariable<double>(Values{1e-12});
  auto f = makeVariable<float>(Values{1e-12});
  auto base_d = makeVariable<double>(Values{1.0});
  auto base_f = makeVariable<float>(Values{1.0});
  auto op = [](const auto a, const auto b) { return a + b; };
  const auto sum_fd =
      transform<pair_custom_t<std::tuple<float, double>>>(base_f, d, op);
  const auto sum_dd =
      transform<pair_custom_t<std::tuple<double, double>>>(base_d, d, op);
  EXPECT_NE(sum_fd.values<double>()[0], 1.0f);
  EXPECT_EQ(sum_fd.values<double>()[0], 1.0f + 1e-12);
  EXPECT_NE(sum_dd.values<double>()[0], 1.0);
  EXPECT_EQ(sum_dd.values<double>()[0], 1.0 + 1e-12);
  const auto sum_ff =
      transform<pair_custom_t<std::tuple<float, float>>>(base_f, f, op);
  const auto sum_df =
      transform<pair_custom_t<std::tuple<double, float>>>(base_d, f, op);
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
  transform_in_place<pair_custom_t<std::tuple<float, double>>>(sum_f, d, op);
  transform_in_place<pair_custom_t<std::tuple<double, double>>>(sum_d, d, op);
  EXPECT_EQ(sum_f.values<float>()[0], 1.0f);
  EXPECT_NE(sum_d.values<double>()[0], 1.0);
  EXPECT_EQ(sum_d.values<double>()[0], 1.0 + 1e-12);
  transform_in_place<pair_custom_t<std::tuple<float, float>>>(sum_f, f, op);
  transform_in_place<pair_custom_t<std::tuple<double, float>>>(sum_d, f, op);
  EXPECT_EQ(sum_f.values<float>()[0], 1.0f);
  EXPECT_NE(sum_d.values<double>()[0], 1.0 + 1e-12);
  EXPECT_EQ(sum_d.values<double>()[0], 1.0 + 1e-12 + 1e-12);
}

TEST(TransformTest, combined_uncertainty_propagation) {
  auto a =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{2.0}, Variances{0.1});
  auto a_2_step(a);
  const auto b = makeVariable<double>(Values{3.0}, Variances{0.2});

  const auto abb = transform<pair_self_t<double>>(
      a, b, [](const auto &x, const auto &y) { return x * y + y; });
  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &x, const auto y) { x = x * y + y; });
  transform_in_place<pair_self_t<double>>(
      a_2_step, b, [](auto &x, const auto y) { x = x * y; });
  transform_in_place<pair_self_t<double>>(
      a_2_step, b, [](auto &x, const auto y) { x = x + y; });

  EXPECT_TRUE(equals(a.values<double>(), {2.0 * 3.0 + 3.0}));
  EXPECT_TRUE(equals(a.variances<double>(), {0.1 * 3 * 3 + 0.2 * 2 * 2 + 0.2}));
  EXPECT_EQ(abb, a);
  EXPECT_EQ(abb, a_2_step);
}

TEST(TransformTest, unary_on_event_list) {
  auto a = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto a_ = a.values<event_list<double>>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<event_list<double>>(
      a, overloaded{[](auto &x) { x.clear(); }, [](units::Unit &) {}});
  EXPECT_TRUE(a_[0].empty());
  EXPECT_TRUE(a_[1].empty());
}

TEST(TransformTest, unary_on_event_list_with_variance) {
  Dimensions dims(Dim::Y, 2);
  auto a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(), event_list<double>()},
      Variances{event_list<double>(), event_list<double>()});
  auto vals = a.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = a.variances<event_list<double>>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {4.4};

  transform_in_place<event_list<double>>(
      a, overloaded{[](auto &x) { x.clear(); }, [](units::Unit &) {}});
  EXPECT_TRUE(vals[0].empty());
  EXPECT_TRUE(vals[1].empty());
  EXPECT_TRUE(vars[0].empty());
  EXPECT_TRUE(vars[1].empty());
}

TEST(TransformTest, unary_on_event_list_with_variance_size_fail) {
  Dimensions dims(Dim::Y, 2);
  auto a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(), event_list<double>()},
      Variances{event_list<double>(), event_list<double>()});
  auto vals = a.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = a.variances<event_list<double>>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {};
  const auto expected(a);

  // If an exception occures due to a size mismatch between values and variances
  // we give a strong exception guarantee, i.e., data is untouched. Note that
  // there is no such guarantee if an exception occurs in the provided lambda.
  ASSERT_THROW(
      transform_in_place<event_list<double>>(
          a, overloaded{[](auto &x) { x.clear(); }, [](units::Unit &) {}}),
      except::SizeError);
  EXPECT_EQ(a, expected);
}

TEST(TransformTest, binary_on_event_list_with_variance_size_fail) {
  Dimensions dims(Dim::Y, 2);
  auto a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(), event_list<double>()},
      Variances{event_list<double>(), event_list<double>()});
  auto vals = a.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = a.variances<event_list<double>>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {};
  const auto expected(a);

  // If an exception occures due to a size mismatch between values and variances
  // we give a strong exception guarantee, i.e., data is untouched. Note that
  // there is no such guarantee if an exception occurs in the provided lambda.
  ASSERT_THROW(transform_in_place<pair_self_t<event_list<double>>>(
                   a, a,
                   overloaded{[](auto &x, const auto &) { x.clear(); },
                              [](units::Unit &, const units::Unit &) {}}),
               except::SizeError);
  EXPECT_EQ(a, expected);
}

TEST(TransformTest, binary_on_event_list_with_variance_accepts_size_mismatch) {
  Dimensions dims(Dim::Y, 2);
  auto a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(), event_list<double>()},
      Variances{event_list<double>(), event_list<double>()});
  const auto expected(a);
  auto vals = a.values<event_list<double>>();
  auto vars = a.variances<event_list<double>>();
  vals[0] = {1, 2, 3};
  vars[0] = {1.1, 2.2, 3.3};
  const auto b(a);
  vals[1] = {4};
  vars[1] = {4.4};

  // Size mismatch between a and b is allowed for a user-defined operation on
  // the event list.
  ASSERT_NO_THROW(transform_in_place<pair_self_t<event_list<double>>>(
      a, b,
      overloaded{[](auto &x, const auto &) { x.clear(); },
                 [](units::Unit &, const units::Unit &) {}}));
  EXPECT_EQ(a, expected);
}

class TransformTest_events_binary_values_variances_size_fail
    : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable a = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(2), event_list<double>(2)},
      Variances{event_list<double>(2), event_list<double>(2)});
  Variable val_var = a;
  Variable val = makeVariable<event_list<double>>(
      Dimensions(dims), Values{event_list<double>(2), event_list<double>(2)});
  static constexpr auto op = [](const auto i, const auto j) { return i * j; };
  static constexpr auto op_in_place = [](auto &i, const auto j) { i *= j; };
};

TEST_F(TransformTest_events_binary_values_variances_size_fail, baseline) {
  ASSERT_NO_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op));
  ASSERT_NO_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op));
  ASSERT_NO_THROW(
      transform_in_place<pair_self_t<double>>(a, val_var, op_in_place));
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place));
}

TEST_F(TransformTest_events_binary_values_variances_size_fail,
       a_values_size_bad) {
  a.values<event_list<double>>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_events_binary_values_variances_size_fail,
       a_variances_size_bad) {
  a.variances<event_list<double>>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_events_binary_values_variances_size_fail,
       val_var_values_size_bad) {
  val_var.values<event_list<double>>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_events_binary_values_variances_size_fail,
       val_var_variances_size_bad) {
  val_var.variances<event_list<double>>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_events_binary_values_variances_size_fail,
       val_values_size_bad) {
  val.values<event_list<double>>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place),
               except::SizeError);
}

TEST_F(TransformBinaryTest, events_val_var_with_events_val_var) {
  auto a = make_events_variable_with_variance<double>();
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  auto b = make_events_variable_with_variance<double>();
  set_events_values<double>(b, {{0.1, 0.2, 0.3}, {0.4}});
  set_events_variances<double>(b, {{0.5, 0.6, 0.7}, {0.8}});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  // We rely on correctness of *dense* operations (Variable multiplcation is
  // also built on transform).
  auto a0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3},
                                 Variances{5, 6, 7});
  auto b0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0.1, 0.2, 0.3},
                                 Variances{0.5, 0.6, 0.7});
  auto a1 =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{4}, Variances{8});
  auto b1 =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{0.4}, Variances{0.8});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[0], expected0.values<double>()));
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[1], expected1.values<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[0],
                     expected0.variances<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[1],
                     expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, events_val_var_with_events_val) {
  auto a = make_events_variable_with_variance<double>();
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  auto b = make_events_variable<double>();
  set_events_values<double>(b, {{0.1, 0.2, 0.3}, {0.4}});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  auto a0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3},
                                 Variances{5, 6, 7});
  auto b0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0.1, 0.2, 0.3});
  auto a1 =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{4}, Variances{8});
  auto b1 = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{0.4});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[0], expected0.values<double>()));
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[1], expected1.values<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[0],
                     expected0.variances<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[1],
                     expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, events_val_var_with_val_var) {
  auto a = make_events_variable_with_variance<double>();
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.5, 1.6},
                                Variances{1.7, 1.8});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  auto a0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3},
                                 Variances{5, 6, 7});
  auto b0 = makeVariable<double>(Values{1.5}, Variances{1.7});
  auto a1 =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{4}, Variances{8});
  auto b1 = makeVariable<double>(Values{1.6}, Variances{1.8});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[0], expected0.values<double>()));
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[1], expected1.values<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[0],
                     expected0.variances<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[1],
                     expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, events_val_var_with_val) {
  auto a = make_events_variable_with_variance<double>();
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.5, 1.6});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  auto a0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3},
                                 Variances{5, 6, 7});
  auto b0 = makeVariable<double>(Values{1.5});
  auto a1 =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{4}, Variances{8});
  auto b1 = makeVariable<double>(Values{1.6});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[0], expected0.values<double>()));
  EXPECT_TRUE(
      equals(a.values<event_list<double>>()[1], expected1.values<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[0],
                     expected0.variances<double>()));
  EXPECT_TRUE(equals(a.variances<event_list<double>>()[1],
                     expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, broadcast_events_val_var_with_val) {
  auto a = make_events_variable_with_variance<double>();
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  const auto b = makeVariable<float>(Dims{Dim::Z}, Shape{2}, Values{1.5, 1.6});

  const auto ab = transform<pair_custom_t<std::tuple<double, float>>>(a, b, op);

  auto a0 = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3},
                                 Variances{5, 6, 7});
  auto b0 = makeVariable<float>(Values{1.5});
  auto a1 =
      makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{4}, Variances{8});
  auto b1 = makeVariable<float>(Values{1.6});
  auto expected00 = a0 * b0;
  auto expected01 = a0 * b1;
  auto expected10 = a1 * b0;
  auto expected11 = a1 * b1;
  const auto vals = ab.values<event_list<double>>();
  const auto vars = ab.variances<event_list<double>>();
  EXPECT_TRUE(equals(vals[0], expected00.values<double>()));
  EXPECT_TRUE(equals(vals[1], expected01.values<double>()));
  EXPECT_TRUE(equals(vals[2], expected10.values<double>()));
  EXPECT_TRUE(equals(vals[3], expected11.values<double>()));
  EXPECT_TRUE(equals(vars[0], expected00.variances<double>()));
  EXPECT_TRUE(equals(vars[1], expected01.variances<double>()));
  EXPECT_TRUE(equals(vars[2], expected10.variances<double>()));
  EXPECT_TRUE(equals(vars[3], expected11.variances<double>()));

  EXPECT_EQ(ab.dims(), Dimensions({Dim::Y, Dim::Z}, {2, 2}));
}

// Currently transform_in_place supports outputs with fewer dimensions than the
// other arguments, effectively applying the same operation multiple times to
// the same output element. This is useful, e.g., when implementing sums or
// integrations, but may be unexpected. Should we fail and support this as a
// separate operation instead?
TEST_F(TransformBinaryTest, fail_dimension_reduction) {
  auto a = make_events_variable_with_variance<double>();
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  const auto b = makeVariable<float>(Dims{Dim::Z}, Shape{2}, Values{1.5, 1.6});

  EXPECT_THROW((transform_in_place<pair_custom_t<std::tuple<double, float>>>(
                   a, b, op_in_place)),
               except::NotFoundError);
}

// It is possible to use transform with functors that call non-built-in
// functions. To do so we have to define that function for the ValueAndVariance
// helper. If this turns out to be a useful feature we should move
// ValueAndVariance out of the `detail` namespace and document the mechanism.
constexpr auto user_op(const double) { return 123.0; }
constexpr auto user_op(const ValueAndVariance<double>) {
  return ValueAndVariance<double>{123.0, 456.0};
}
constexpr auto user_op(const units::Unit &) { return units::s; }

TEST(TransformTest, user_op_with_variances) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m,
                                  Values{1.1, 2.2}, Variances{1.1, 3.0});

  const auto result = transform<double>(var, [](auto x) { return user_op(x); });
  transform_in_place<double>(var, [](auto &x) { x = user_op(x); });

  auto expected = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::s,
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
  auto a =
      makeVariable<double>(Dims(), Shape(), units::Unit(units::m * units::m));
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<double>(a, unary),
               std::runtime_error);
  EXPECT_EQ(a, original);
  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(a, a, binary),
               std::runtime_error);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, slice_unit_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<double>(a.slice({Dim::X, 0}), unary),
               except::UnitError);
  EXPECT_EQ(a, original);
  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(
                   a.slice({Dim::X, 0}), a.slice({Dim::X, 0}), binary),
               except::UnitError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, dimensions_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  auto b = makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m);
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(a, b, binary),
               std::runtime_error);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, variances_fail) {
  auto a = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  auto b = makeVariable<double>(Dimensions{Dim::X, 2}, units::m, Values{},
                                Variances{});
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(a, b, binary),
               std::runtime_error);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, events_variance_length_fail) {
  auto a = make_events_variable_with_variance<double>();
  a.setUnit(units::m);
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8, 9}});
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<double>(a, unary),
               except::SizeError);
  EXPECT_EQ(a, original);
  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(a, a, binary),
               except::SizeError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, events_length_fail) {
  auto a = make_events_variable_with_variance<double>();
  a.setUnit(units::m);
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  auto b = make_events_variable_with_variance<double>();
  b.setUnit(units::m);
  set_events_values<double>(b, {{1, 2, 3}, {4, 5}});
  set_events_variances<double>(b, {{5, 6, 7}, {8, 9}});
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(a, b, binary),
               except::SizeError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, events_no_variances_length_fail) {
  auto a = make_events_variable<double>();
  a.setUnit(units::m);
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  auto b = make_events_variable<double>();
  b.setUnit(units::m);
  set_events_values<double>(b, {{1, 2, 3}, {4, 5}});
  const auto original(a);

  EXPECT_THROW(dry_run::transform_in_place<pair_self_t<double>>(a, b, binary),
               except::SizeError);
  EXPECT_EQ(a, original);
}

TEST_F(TransformInPlaceDryRunTest, unchanged_if_success) {
  auto a = make_events_variable_with_variance<double>();
  a.setUnit(units::m);
  set_events_values<double>(a, {{1, 2, 3}, {4}});
  set_events_variances<double>(a, {{5, 6, 7}, {8}});
  const auto original(a);

  dry_run::transform_in_place<double>(a, unary);
  EXPECT_EQ(a, original);
  dry_run::transform_in_place<pair_self_t<double>>(a, a, binary);
  EXPECT_EQ(a, original);
}

TEST(TransformFlagsTest, no_variance_on_arg) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  auto binary_op = [](auto x, auto y) { return x + y; };
  auto op_arg_0_has_flags =
      scipp::overloaded{transform_flags::expect_variance_arg<0>, binary_op};
  Variable out;
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_no_variance,
                                           op_arg_0_has_flags)));
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_no_variance, var_with_variance, op_arg_0_has_flags)),
               except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           op_arg_0_has_flags)));
  auto op_arg_1_has_flags =
      scipp::overloaded{transform_flags::expect_variance_arg<1>, binary_op};
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_with_variance, var_no_variance, op_arg_1_has_flags)),
               except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_with_variance,
                                           op_arg_1_has_flags)));
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           op_arg_1_has_flags)));
  auto all_args_with_flag =
      scipp::overloaded{transform_flags::expect_variance_arg<0>,
                        transform_flags::expect_variance_arg<1>, binary_op};
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_no_variance, var_no_variance, all_args_with_flag)),
               except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_with_variance,
                                           all_args_with_flag)));
}

TEST(TransformFlagsTest, no_variance_on_arg_in_place) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  auto unary_in_place = [](auto &, auto) {};
  auto op_arg_0_has_flags = scipp::overloaded{
      transform_flags::expect_variance_arg<0>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_no_variance, op_arg_0_has_flags),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op_arg_0_has_flags));
  auto op_arg_1_has_flags = scipp::overloaded{
      transform_flags::expect_variance_arg<1>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_no_variance, op_arg_1_has_flags),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op_arg_1_has_flags));
}

TEST(TransformFlagsTest, variance_on_arg) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  auto binary_op = [](auto x, auto y) { return x + y; };
  auto op_arg_0_has_flags =
      scipp::overloaded{transform_flags::expect_no_variance_arg<0>, binary_op};
  Variable out;
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_with_variance, var_no_variance, op_arg_0_has_flags)),
               except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_no_variance, var_with_variance,
                                           op_arg_0_has_flags)));
  EXPECT_NO_THROW((out = transform<std::tuple<double>>(
                       var_no_variance, var_no_variance, op_arg_0_has_flags)));
  auto op_arg_1_has_flags =
      scipp::overloaded{transform_flags::expect_no_variance_arg<1>, binary_op};
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_no_variance, var_with_variance, op_arg_1_has_flags)),
               except::VariancesError);
  EXPECT_NO_THROW(
      (out = transform<std::tuple<double>>(var_with_variance, var_no_variance,
                                           op_arg_1_has_flags)));
  EXPECT_NO_THROW((out = transform<std::tuple<double>>(
                       var_no_variance, var_no_variance, op_arg_1_has_flags)));
  auto all_args_with_flag =
      scipp::overloaded{transform_flags::expect_no_variance_arg<0>,
                        transform_flags::expect_no_variance_arg<1>, binary_op};
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_with_variance, var_with_variance, all_args_with_flag)),
               except::VariancesError);
  EXPECT_NO_THROW((out = transform<std::tuple<double>>(
                       var_no_variance, var_no_variance, all_args_with_flag)));
}

TEST(TransformFlagsTest, no_out_variance) {
  constexpr auto op =
      overloaded{transform_flags::no_out_variance, element::arg_list<double>,
                 [](const auto) { return true; },
                 [](const units::Unit &) { return units::one; }};
  const auto var = makeVariable<double>(Values{1.0}, Variances{1.0});
  EXPECT_EQ(transform(var, op), true * units::one);
}

TEST(TransformFlagsTest, variance_on_arg_in_place) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  auto unary_in_place = [](auto &, auto) {};
  auto op_arg_0_has_flags = scipp::overloaded{
      transform_flags::expect_no_variance_arg<0>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_with_variance, var_with_variance, op_arg_0_has_flags),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op_arg_0_has_flags));
  auto op_arg_1_has_flags = scipp::overloaded{
      transform_flags::expect_no_variance_arg<1>, unary_in_place};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_with_variance, var_with_variance, op_arg_1_has_flags),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op_arg_1_has_flags));
}

TEST(TransformFlagsTest, expect_in_variance_if_out_variance) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  constexpr auto inplace_op = [](auto &&x, const auto &y) { x += y; };
  constexpr auto op = overloaded{
      transform_flags::expect_in_variance_if_out_variance, inplace_op};
  EXPECT_THROW(transform_in_place<std::tuple<double>>(var_with_variance,
                                                      var_no_variance, op),
               except::VariancesError);
  EXPECT_THROW(transform_in_place<std::tuple<double>>(var_no_variance,
                                                      var_with_variance, op),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(var_no_variance,
                                                         var_no_variance, op));
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op));
}

TEST(TransformFlagsTest, expect_all_or_none_have_variance) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  auto binary_op = [](auto x, auto y) { return x + y; };
  auto op_has_flags = scipp::overloaded{
      transform_flags::expect_all_or_none_have_variance, binary_op};
  Variable out;
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_with_variance, var_no_variance, op_has_flags)),
               except::VariancesError);
  EXPECT_THROW((out = transform<std::tuple<double>>(
                    var_no_variance, var_with_variance, op_has_flags)),
               except::VariancesError);
  EXPECT_NO_THROW((out = transform<std::tuple<double>>(
                       var_no_variance, var_no_variance, op_has_flags)));
  EXPECT_NO_THROW((out = transform<std::tuple<double>>(
                       var_with_variance, var_with_variance, op_has_flags)));
}

TEST(TransformFlagsTest, expect_all_or_none_have_variance_in_place) {
  auto var_with_variance = makeVariable<double>(Values{1}, Variances{1});
  auto var_no_variance = makeVariable<double>(Values{1});
  auto unary_op = [](auto &, auto) {};
  auto op_has_flags = scipp::overloaded{
      transform_flags::expect_all_or_none_have_variance, unary_op};
  Variable out;
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_with_variance, var_no_variance, op_has_flags),
               except::VariancesError);
  EXPECT_THROW(transform_in_place<std::tuple<double>>(
                   var_no_variance, var_with_variance, op_has_flags),
               except::VariancesError);
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_no_variance, var_no_variance, op_has_flags));
  EXPECT_NO_THROW(transform_in_place<std::tuple<double>>(
      var_with_variance, var_with_variance, op_has_flags));
}

TEST(TransformEigenTest, is_eigen_type_test) {
  EXPECT_TRUE(scipp::variable::detail::is_eigen_type_v<Eigen::Vector3d>);
  EXPECT_TRUE(scipp::variable::detail::is_eigen_type_v<Eigen::Matrix3d>);
}
