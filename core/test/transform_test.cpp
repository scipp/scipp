// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/dimensions.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

class TransformUnaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{
      overloaded{[](auto &x) { x *= 2.0; }, [](units::Unit &) {}}};
  static constexpr auto op{
      overloaded{[](const auto x) { return x * 2.0; },
                 [](const units::Unit &unit) { return unit; }}};
};

TEST_F(TransformUnaryTest, dense) {
  auto var = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(var.values<double>(), {1.1 * 2.0, 2.2 * 2.0}));
  // In-place transform used to check result of non-in-place transform.
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, dense_with_variances) {
  auto var = makeVariable<double>({Dim::X, 2}, {1.1, 2.2}, {1.1, 3.0});

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(var.values<double>(), {2.2, 4.4}));
  EXPECT_TRUE(equals(var.variances<double>(), {4.4, 12.0}));
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, elements_of_sparse) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  const auto result = transform<double>(var, op);
  transform_in_place<double>(var, op_in_place);

  EXPECT_TRUE(equals(vals[0], {1 * 2.0, 2 * 2.0, 3 * 2.0}));
  EXPECT_TRUE(equals(vals[1], {4 * 2.0}));
  EXPECT_EQ(result, var);
}

TEST_F(TransformUnaryTest, elements_of_sparse_with_variance) {
  auto var = makeVariableWithVariances<double>(
      {{Dim::Y, Dim::X}, {2, Dimensions::Sparse}});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = var.sparseVariances<double>();
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

TEST_F(TransformUnaryTest, sparse_values_variances_size_fail) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(1)},
      {sparse_container<double>(2), sparse_container<double>(2)});

  ASSERT_THROW_NODISCARD(transform<double>(a, op), except::SizeError);
  ASSERT_THROW(transform_in_place<double>(a, op_in_place), except::SizeError);
  a.sparseVariances<double>()[1].resize(1);
  ASSERT_NO_THROW_NODISCARD(transform<double>(a, op));
  ASSERT_NO_THROW(transform_in_place<double>(a, op_in_place));
}

TEST_F(TransformUnaryTest, in_place_unit_change) {
  const auto var = makeVariable<double>({Dim::X, 2}, units::m, {1.0, 2.0});
  const auto expected =
      makeVariable<double>({Dim::X, 2}, units::m * units::m, {1.0, 4.0});
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
  const auto var = makeVariable<float>({Dim::X, 2}, {1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out = transform<float>(
      var, overloaded{[](const auto x) { return -1.0 * x; },
                      [](const units::Unit &unit) { return unit; }});
  EXPECT_TRUE(equals(out.values<double>(), {-1.1f, -2.2f}));
}

TEST(TransformTest, apply_unary_dtype_preserved) {
  const auto varD = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto varF = makeVariable<float>({Dim::X, 2}, {1.1, 2.2});
  auto outD = transform<double, float>(varD, [](const auto x) { return -x; });
  auto outF = transform<double, float>(varF, [](const auto x) { return -x; });
  EXPECT_TRUE(equals(outD.values<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.values<float>(), {-1.1f, -2.2f}));
}

class TransformBinaryTest : public ::testing::Test {
protected:
  static constexpr auto op_in_place{[](auto &x, const auto &y) { x *= y; }};
  static constexpr auto op{[](const auto &x, const auto &y) { return x * y; }};
};

TEST_F(TransformBinaryTest, dense) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>(3.3);

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  const auto ba = transform<pair_self_t<double>>(b, a, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3, 2.2 * 3.3}));
  EXPECT_EQ(ab, ba);
  EXPECT_EQ(ab, a);
  EXPECT_EQ(ba, a);
}

TEST_F(TransformBinaryTest, dims_and_shape_fail_in_place) {
  auto a = makeVariable<double>({Dim::X, 2});
  auto b = makeVariable<double>({Dim::Y, 2});
  auto c = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});

  EXPECT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place));
  EXPECT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, c, op_in_place));
}

TEST_F(TransformBinaryTest, dims_and_shape_fail) {
  auto a = makeVariable<double>({Dim::X, 4});
  auto b = makeVariable<double>({Dim::X, 2});
  auto c = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});

  EXPECT_ANY_THROW(transform<pair_self_t<double>>(a, b, op));
  EXPECT_ANY_THROW(transform<pair_self_t<double>>(a, c, op));
}

TEST_F(TransformBinaryTest, dense_mixed_type) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<float>(3.3);

  const auto ab = transform<pair_custom_t<std::pair<double, float>>>(a, b, op);
  const auto ba = transform<pair_custom_t<std::pair<float, double>>>(b, a, op);
  transform_in_place<pair_custom_t<std::pair<double, float>>>(a, b,
                                                              op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3f, 2.2 * 3.3f}));
  EXPECT_EQ(ab, ba);
  EXPECT_EQ(ab, a);
  EXPECT_EQ(ba, a);
}

TEST_F(TransformBinaryTest, var_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});

  auto ab = transform<pair_self_t<double>>(a, b.slice({Dim::Y, 1}), op);
  transform_in_place<pair_self_t<double>>(a, b.slice({Dim::Y, 1}), op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1 * 3.3, 2.2 * 3.3}));
  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_without_variance) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  Variable slice_copy(a.slice({Dim::X, 1}));
  auto reference = a * slice_copy;
  transform_in_place<pair_self_t<double>>(a, a.slice({Dim::X, 1}), op_in_place);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, in_place_self_overlap_with_variance) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2}, {1.0, 2.0});
  Variable slice_copy(a.slice({Dim::X, 1}));
  auto reference = a * slice_copy;
  // With self-overlap the implementation needs to make a copy of the rhs. This
  // is a regression test: An initial implementation was unintentionally
  // dropping the variances when making that copy.
  transform_in_place<pair_self_t<double>>(a, a.slice({Dim::X, 1}), op_in_place);
  ASSERT_EQ(a, reference);
}

TEST_F(TransformBinaryTest, view_with_var) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>(3.3);

  transform_in_place<pair_self_t<double>>(a.slice({Dim::X, 1}), b, op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1, 2.2 * 3.3}));
}

TEST_F(TransformBinaryTest, view_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});

  transform_in_place<pair_self_t<double>>(a.slice({Dim::X, 1}),
                                          b.slice({Dim::Y, 1}), op_in_place);

  EXPECT_TRUE(equals(a.values<double>(), {1.1, 2.2 * 3.3}));
}

TEST_F(TransformBinaryTest, dense_sparse) {
  auto sparse = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto sparse_ = sparse.sparseValues<double>();
  sparse_[0] = {1, 2, 3};
  sparse_[1] = {4};
  auto dense = makeVariable<double>({Dim::Y, 2}, {1.5, 0.5});

  const auto ab = transform<pair_self_t<double>>(sparse, dense, op);
  const auto ba = transform<pair_self_t<double>>(dense, sparse, op);
  transform_in_place<pair_self_t<double>>(sparse, dense, op_in_place);

  EXPECT_TRUE(equals(sparse_[0], {1.5, 3.0, 4.5}));
  EXPECT_TRUE(equals(sparse_[1], {2.0}));
  EXPECT_EQ(ab, sparse);
  EXPECT_EQ(ba, sparse);
}

TEST_F(TransformBinaryTest, sparse_size_fail) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(1)});
  auto b = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>()});

  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, b, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place),
               except::SizeError);
  b.sparseValues<double>()[1].resize(1);
  ASSERT_NO_THROW_NODISCARD(transform<pair_self_t<double>>(a, b, op));
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place));
  b.sparseValues<double>()[1].resize(2);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, b, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, b, op_in_place),
               except::SizeError);
}

TEST_F(TransformBinaryTest, in_place_unit_change) {
  const auto var = makeVariable<double>({Dim::X, 2}, units::m, {1.0, 2.0});
  const auto expected =
      makeVariable<double>({Dim::X, 2}, units::m * units::m, {1.0, 4.0});
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
  const auto var = makeVariable<double>({Dim::X, 2}, units::m, {1.0, 2.0});
  const auto expected = makeVariable<double>(3.0);
  auto op_ = [](auto &&a, auto &&b) { a += b; };
  Variable result;

  // Note how accumulate is ignoring the unit.
  result = makeVariable<double>({});
  accumulate_in_place<pair_self_t<double>>(result, var, op_);
  EXPECT_EQ(result, expected);
}

TEST(TransformTest, Eigen_Vector3d_pass_by_value) {
  const auto var = makeVariable<Eigen::Vector3d>(
      {Dim::X, 2},
      {Eigen::Vector3d{1.1, 2.2, 3.3}, Eigen::Vector3d{0.1, 0.2, 0.3}});
  const auto expected =
      makeVariable<Eigen::Vector3d>({}, {Eigen::Vector3d{1.0, 2.0, 3.0}});
  // Passing Eigen types by value often causes issues, ensure that it works.
  auto op = [](const auto x, const auto y) { return x - y; };

  const auto result = transform<pair_self_t<Eigen::Vector3d>>(
      var.slice({Dim::X, 0}), var.slice({Dim::X, 1}), op);

  EXPECT_EQ(result, expected);
}

TEST(TransformTest, mixed_precision) {
  auto d = makeVariable<double>(1e-12);
  auto f = makeVariable<float>(1e-12);
  auto base_d = makeVariable<double>(1.0);
  auto base_f = makeVariable<float>(1.0);
  auto op = [](const auto a, const auto b) { return a + b; };
  const auto sum_fd =
      transform<pair_custom_t<std::pair<float, double>>>(base_f, d, op);
  const auto sum_dd =
      transform<pair_custom_t<std::pair<double, double>>>(base_d, d, op);
  EXPECT_NE(sum_fd.values<double>()[0], 1.0f);
  EXPECT_EQ(sum_fd.values<double>()[0], 1.0f + 1e-12);
  EXPECT_NE(sum_dd.values<double>()[0], 1.0);
  EXPECT_EQ(sum_dd.values<double>()[0], 1.0 + 1e-12);
  const auto sum_ff =
      transform<pair_custom_t<std::pair<float, float>>>(base_f, f, op);
  const auto sum_df =
      transform<pair_custom_t<std::pair<double, float>>>(base_d, f, op);
  EXPECT_EQ(sum_ff.values<float>()[0], 1.0f);
  EXPECT_EQ(sum_ff.values<float>()[0], 1.0f + 1e-12f);
  EXPECT_NE(sum_df.values<double>()[0], 1.0);
  EXPECT_EQ(sum_df.values<double>()[0], 1.0 + 1e-12f);
}

TEST(TransformTest, mixed_precision_in_place) {
  auto d = makeVariable<double>(1e-12);
  auto f = makeVariable<float>(1e-12);
  auto sum_d = makeVariable<double>(1.0);
  auto sum_f = makeVariable<float>(1.0);
  auto op = [](auto &a, const auto b) { a += b; };
  transform_in_place<pair_custom_t<std::pair<float, double>>>(sum_f, d, op);
  transform_in_place<pair_custom_t<std::pair<double, double>>>(sum_d, d, op);
  EXPECT_EQ(sum_f.values<float>()[0], 1.0f);
  EXPECT_NE(sum_d.values<double>()[0], 1.0);
  EXPECT_EQ(sum_d.values<double>()[0], 1.0 + 1e-12);
  transform_in_place<pair_custom_t<std::pair<float, float>>>(sum_f, f, op);
  transform_in_place<pair_custom_t<std::pair<double, float>>>(sum_d, f, op);
  EXPECT_EQ(sum_f.values<float>()[0], 1.0f);
  EXPECT_NE(sum_d.values<double>()[0], 1.0 + 1e-12);
  EXPECT_EQ(sum_d.values<double>()[0], 1.0 + 1e-12 + 1e-12);
}

TEST(TransformTest, combined_uncertainty_propagation) {
  auto a = makeVariable<double>({Dim::X, 1}, {2.0}, {0.1});
  auto a_2_step(a);
  const auto b = makeVariable<double>(3.0, 0.2);

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

TEST(TransformTest, unary_on_sparse_container) {
  auto a = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_ = a.sparseValues<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<sparse_container<double>>(
      a, overloaded{[](auto &x) { x.clear(); }, [](units::Unit &) {}});
  EXPECT_TRUE(a_[0].empty());
  EXPECT_TRUE(a_[1].empty());
}

TEST(TransformTest, unary_on_sparse_container_with_variance) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(), sparse_container<double>()},
      {sparse_container<double>(), sparse_container<double>()});
  auto vals = a.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = a.sparseVariances<double>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {4.4};

  transform_in_place<sparse_container<double>>(
      a, overloaded{[](auto &x) { x.clear(); }, [](units::Unit &) {}});
  EXPECT_TRUE(vals[0].empty());
  EXPECT_TRUE(vals[1].empty());
  EXPECT_TRUE(vars[0].empty());
  EXPECT_TRUE(vars[1].empty());
}

TEST(TransformTest, unary_on_sparse_container_with_variance_size_fail) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(), sparse_container<double>()},
      {sparse_container<double>(), sparse_container<double>()});
  auto vals = a.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = a.sparseVariances<double>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {};
  const auto expected(a);

  // If an exception occures due to a size mismatch between values and variances
  // we give a strong exception guarantee, i.e., data is untouched. Note that
  // there is no such guarantee if an exception occurs in the provided lambda.
  ASSERT_THROW(
      transform_in_place<sparse_container<double>>(
          a, overloaded{[](auto &x) { x.clear(); }, [](units::Unit &) {}}),
      except::SizeError);
  EXPECT_EQ(a, expected);
}

TEST(TransformTest, binary_on_sparse_container_with_variance_size_fail) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(), sparse_container<double>()},
      {sparse_container<double>(), sparse_container<double>()});
  auto vals = a.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};
  auto vars = a.sparseVariances<double>();
  vars[0] = {1.1, 2.2, 3.3};
  vars[1] = {};
  const auto expected(a);

  // If an exception occures due to a size mismatch between values and variances
  // we give a strong exception guarantee, i.e., data is untouched. Note that
  // there is no such guarantee if an exception occurs in the provided lambda.
  ASSERT_THROW(transform_in_place<pair_self_t<sparse_container<double>>>(
                   a, a,
                   overloaded{[](auto &x, const auto &) { x.clear(); },
                              [](units::Unit &, const units::Unit &) {}}),
               except::SizeError);
  EXPECT_EQ(a, expected);
}

TEST(TransformTest,
     binary_on_sparse_container_with_variance_accepts_size_mismatch) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(), sparse_container<double>()},
      {sparse_container<double>(), sparse_container<double>()});
  const auto expected(a);
  auto vals = a.sparseValues<double>();
  auto vars = a.sparseVariances<double>();
  vals[0] = {1, 2, 3};
  vars[0] = {1.1, 2.2, 3.3};
  const auto b(a);
  vals[1] = {4};
  vars[1] = {4.4};

  // Size mismatch between a and b is allowed for a user-defined operation on
  // the sparse container.
  ASSERT_NO_THROW(transform_in_place<pair_self_t<sparse_container<double>>>(
      a, b,
      overloaded{[](auto &x, const auto &) { x.clear(); },
                 [](units::Unit &, const units::Unit &) {}}));
  EXPECT_EQ(a, expected);
}

class TransformTest_sparse_binary_values_variances_size_fail
    : public ::testing::Test {
protected:
  Dimensions dims{{Dim::Y, Dim::X}, {2, Dimensions::Sparse}};
  Variable a = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(2)},
      {sparse_container<double>(2), sparse_container<double>(2)});
  Variable val_var = a;
  Variable val = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(2)});
  static constexpr auto op = [](const auto i, const auto j) { return i * j; };
  static constexpr auto op_in_place = [](auto &i, const auto j) { i *= j; };
};

TEST_F(TransformTest_sparse_binary_values_variances_size_fail, baseline) {
  ASSERT_NO_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op));
  ASSERT_NO_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op));
  ASSERT_NO_THROW(
      transform_in_place<pair_self_t<double>>(a, val_var, op_in_place));
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place));
}

TEST_F(TransformTest_sparse_binary_values_variances_size_fail,
       a_values_size_bad) {
  a.sparseValues<double>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_sparse_binary_values_variances_size_fail,
       a_variances_size_bad) {
  a.sparseVariances<double>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_sparse_binary_values_variances_size_fail,
       val_var_values_size_bad) {
  val_var.sparseValues<double>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_sparse_binary_values_variances_size_fail,
       val_var_variances_size_bad) {
  val_var.sparseVariances<double>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val_var, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op_in_place),
               except::SizeError);
}

TEST_F(TransformTest_sparse_binary_values_variances_size_fail,
       val_values_size_bad) {
  val.sparseValues<double>()[1].resize(1);
  ASSERT_THROW_NODISCARD(transform<pair_self_t<double>>(a, val, op),
                         except::SizeError);
  ASSERT_THROW(transform_in_place<pair_self_t<double>>(a, val, op_in_place),
               except::SizeError);
}

auto make_sparse_variable_with_variance() {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  return makeVariable<double>(
      dims, {sparse_container<double>(), sparse_container<double>()},
      {sparse_container<double>(), sparse_container<double>()});
}

auto make_sparse_variable() {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  return makeVariable<double>(dims);
}

void set_sparse_values(Variable &var,
                       const std::vector<sparse_container<double>> &data) {
  auto vals = var.sparseValues<double>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

void set_sparse_variances(Variable &var,
                          const std::vector<sparse_container<double>> &data) {
  auto vals = var.sparseVariances<double>();
  for (scipp::index i = 0; i < scipp::size(data); ++i)
    vals[i] = data[i];
}

TEST_F(TransformBinaryTest, sparse_val_var_with_sparse_val_var) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = make_sparse_variable_with_variance();
  set_sparse_values(b, {{0.1, 0.2, 0.3}, {0.4}});
  set_sparse_variances(b, {{0.5, 0.6, 0.7}, {0.8}});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  // We rely on correctness of *dense* operations (Variable multiplcation is
  // also built on transform).
  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<double>({Dim::X, 3}, {0.1, 0.2, 0.3}, {0.5, 0.6, 0.7});
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<double>({Dim::X, 1}, {0.4}, {0.8});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(equals(a.sparseValues<double>()[0], expected0.values<double>()));
  EXPECT_TRUE(equals(a.sparseValues<double>()[1], expected1.values<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[0], expected0.variances<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[1], expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, sparse_val_var_with_sparse_val) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = make_sparse_variable();
  set_sparse_values(b, {{0.1, 0.2, 0.3}, {0.4}});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<double>({Dim::X, 3}, {0.1, 0.2, 0.3});
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<double>({Dim::X, 1}, {0.4});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(equals(a.sparseValues<double>()[0], expected0.values<double>()));
  EXPECT_TRUE(equals(a.sparseValues<double>()[1], expected1.values<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[0], expected0.variances<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[1], expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, sparse_val_var_with_val_var) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = makeVariable<double>({Dim::Y, 2}, {1.5, 1.6}, {1.7, 1.8});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<double>(1.5, 1.7);
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<double>(1.6, 1.8);
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(equals(a.sparseValues<double>()[0], expected0.values<double>()));
  EXPECT_TRUE(equals(a.sparseValues<double>()[1], expected1.values<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[0], expected0.variances<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[1], expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, sparse_val_var_with_val) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = makeVariable<double>({Dim::Y, 2}, {1.5, 1.6});

  const auto ab = transform<pair_self_t<double>>(a, b, op);
  transform_in_place<pair_self_t<double>>(a, b, op_in_place);

  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<double>(1.5);
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<double>(1.6);
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(equals(a.sparseValues<double>()[0], expected0.values<double>()));
  EXPECT_TRUE(equals(a.sparseValues<double>()[1], expected1.values<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[0], expected0.variances<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[1], expected1.variances<double>()));

  EXPECT_EQ(ab, a);
}

TEST_F(TransformBinaryTest, broadcast_sparse_val_var_with_val) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  const auto b = makeVariable<float>({Dim::Z, 2}, {1.5, 1.6});

  const auto ab = transform<pair_custom_t<std::pair<double, float>>>(a, b, op);

  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<float>(1.5);
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<float>(1.6);
  auto expected00 = a0 * b0;
  auto expected01 = a0 * b1;
  auto expected10 = a1 * b0;
  auto expected11 = a1 * b1;
  const auto vals = ab.sparseValues<double>();
  const auto vars = ab.sparseVariances<double>();
  EXPECT_TRUE(equals(vals[0], expected00.values<double>()));
  EXPECT_TRUE(equals(vals[1], expected10.values<double>()));
  EXPECT_TRUE(equals(vals[2], expected01.values<double>()));
  EXPECT_TRUE(equals(vals[3], expected11.values<double>()));
  EXPECT_TRUE(equals(vars[0], expected00.variances<double>()));
  EXPECT_TRUE(equals(vars[1], expected10.variances<double>()));
  EXPECT_TRUE(equals(vars[2], expected01.variances<double>()));
  EXPECT_TRUE(equals(vars[3], expected11.variances<double>()));

  EXPECT_EQ(ab.dims(),
            Dimensions({Dim::Z, Dim::Y, Dim::X}, {2, 2, Dimensions::Sparse}));
}

// Currently transform_in_place supports outputs with fewer dimensions than the
// other arguments, effectively applying the same operation multiple times to
// the same output element. This is useful, e.g., when implementing sums or
// integrations, but may be unexpected. Should we fail and support this as a
// separate operation instead?
TEST_F(TransformBinaryTest, DISABLED_broadcast_sparse_val_var_with_val) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  const auto b = makeVariable<float>({Dim::Z, 2}, {1.5, 1.6});

  EXPECT_THROW((transform_in_place<pair_custom_t<std::pair<double, float>>>(
                   a, b, op_in_place)),
               except::SizeError);
}

// It is possible to use transform with functors that call non-built-in
// functions. To do so we have to define that function for the ValueAndVariance
// helper. If this turns out to be a useful feature we should move
// ValueAndVariance out of the `detail` namespace and document the mechanism.
constexpr auto user_op(const double) { return 123.0; }
constexpr auto user_op(const scipp::core::detail::ValueAndVariance<double>) {
  return scipp::core::detail::ValueAndVariance<double>{123.0, 456.0};
}
constexpr auto user_op(const units::Unit &) { return units::s; }

TEST(TransformTest, user_op_with_variances) {
  auto var =
      makeVariable<double>({Dim::X, 2}, units::m, {1.1, 2.2}, {1.1, 3.0});

  const auto result = transform<double>(var, [](auto x) { return user_op(x); });
  transform_in_place<double>(var, [](auto &x) { x = user_op(x); });

  auto expected =
      makeVariable<double>({Dim::X, 2}, units::s, {123, 123}, {456, 456});
  EXPECT_EQ(result, expected);
  EXPECT_EQ(result, var);
}

TEST(TransformTest, in_place_dry_run) {
  auto a = make_sparse_variable_with_variance();
  a.setUnit(units::m * units::m);
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  const auto original(a);

  dry_run::transform_in_place<double>(a, [](auto &a_) { a_ = sqrt(a_); });
  EXPECT_EQ(a, original);
}
