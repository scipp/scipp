// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "dimensions.h"
#include "transform.h"
#include "variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(TransformTest, apply_unary_in_place) {
  auto var = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  transform_in_place<double>(var, [](auto &x) { x = -x; });
  EXPECT_TRUE(equals(var.values<double>(), {-1.1, -2.2}));
}

TEST(TransformTest, apply_unary_in_place_with_variances) {
  auto var = makeVariable<double>({Dim::X, 2}, {1.1, 2.2}, {1.1, 3.0});
  transform_in_place<double>(var, [](auto &x) { x *= 2.0; });
  EXPECT_TRUE(equals(var.values<double>(), {2.2, 4.4}));
  EXPECT_TRUE(equals(var.variances<double>(), {4.4, 12.0}));
}

TEST(TransformTest, apply_unary_implicit_conversion) {
  const auto var = makeVariable<float>({Dim::X, 2}, {1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out = transform<double, float>(var, [](const double x) { return -x; });
  EXPECT_TRUE(equals(out.values<double>(), {-1.1f, -2.2f}));
}

TEST(TransformTest, apply_unary) {
  const auto varD = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto varF = makeVariable<float>({Dim::X, 2}, {1.1, 2.2});
  auto outD = transform<double, float>(varD, [](const auto x) { return -x; });
  auto outF = transform<double, float>(varF, [](const auto x) { return -x; });
  EXPECT_TRUE(equals(outD.values<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.values<float>(), {-1.1f, -2.2f}));
}

TEST(TransformTest, apply_binary_in_place) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>(3.3);
  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &x, const auto y) { x += y; });
  EXPECT_TRUE(equals(a.values<double>(), {4.4, 5.5}));
}

TEST(TransformTest, apply_binary_in_place_var_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});
  transform_in_place<pair_self_t<double>>(
      a, b(Dim::Y, 1), [](auto &x, const auto y) { x += y; });
  EXPECT_TRUE(equals(a.values<double>(), {4.4, 5.5}));
}

TEST(TransformTest, apply_binary_in_place_view_with_var) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>(3.3);
  transform_in_place<pair_self_t<double>>(
      a(Dim::X, 1), b, [](auto &x, const auto y) { x += y; });
  EXPECT_TRUE(equals(a.values<double>(), {1.1, 5.5}));
}

TEST(TransformTest, apply_binary_in_place_view_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});
  transform_in_place<pair_self_t<double>>(
      a(Dim::X, 1), b(Dim::Y, 1), [](auto &x, const auto y) { x += y; });
  EXPECT_TRUE(equals(a.values<double>(), {1.1, 5.5}));
}

TEST(TransformTest, transform_combines_uncertainty_propgation) {
  auto a = makeVariable<double>({Dim::X, 1}, {2.0}, {0.1});
  const auto b = makeVariable<double>(3.0, 0.2);
  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &x, const auto y) { x = x * y + y; });
  EXPECT_TRUE(equals(a.values<double>(), {2.0 * 3.0 + 3.0}));
  EXPECT_TRUE(equals(a.variances<double>(), {0.1 * 3 * 3 + 0.2 * 2 * 2 + 0.2}));
}

TEST(TransformTest, unary_on_elements_of_sparse) {
  auto a = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<double>(a, [](auto &x) { x = sqrt(x); });
  EXPECT_TRUE(equals(a_[0], {1, 2, 3}));
  EXPECT_TRUE(equals(a_[1], {2}));
}

TEST(TransformTest, unary_on_elements_of_sparse_with_variance) {
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

  transform_in_place<double>(a, [](auto &x) { x *= 2.0; });
  EXPECT_TRUE(equals(vals[0], {2, 4, 6}));
  EXPECT_TRUE(equals(vals[1], {8}));
  EXPECT_TRUE(equals(vars[0], {4.4, 8.8, 13.2}));
  EXPECT_TRUE(equals(vars[1], {17.6}));
}

TEST(TransformTest, unary_on_sparse_container) {
  auto a = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<sparse_container<double>>(a, [](auto &&x) { x.clear(); });
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

  transform_in_place<sparse_container<double>>(a, [](auto &&x) { x.clear(); });
  EXPECT_TRUE(vals[0].empty());
  EXPECT_TRUE(vals[1].empty());
  EXPECT_TRUE(vars[0].empty());
  EXPECT_TRUE(vars[1].empty());
}

TEST(TransformTest, binary_with_dense) {
  auto sparse = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto sparse_ = sparse.sparseSpan<double>();
  sparse_[0] = {1, 2, 3};
  sparse_[1] = {4};
  auto dense = makeVariable<double>({Dim::Y, 2}, {1.5, 0.5});

  transform_in_place<pair_self_t<double>>(
      sparse, dense, [](auto &a, const auto b) { a *= b; });

  EXPECT_TRUE(equals(sparse_[0], {1.5, 3.0, 4.5}));
  EXPECT_TRUE(equals(sparse_[1], {2.0}));
}

TEST(TransformTest, mixed_precision) {
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

TEST(TransformInPlaceTest, sparse_unary_values_variances_size_fail) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(1)},
      {sparse_container<double>(2), sparse_container<double>(2)});
  auto op = [](auto &a) { a *= 2.0; };

  ASSERT_ANY_THROW(transform_in_place<double>(a, op));
  a.sparseVariances<double>()[1].resize(1);
  ASSERT_NO_THROW(transform_in_place<double>(a, op));
}

TEST(TransformInPlaceTest, sparse_binary_size_fail) {
  Dimensions dims({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(1)});
  auto b = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>()});
  auto op = [](auto &a, const auto b) { a *= b; };

  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, b, op));
  b.sparseValues<double>()[1].resize(1);
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, b, op));
  b.sparseValues<double>()[1].resize(2);
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, b, op));
}

class TransformInPlaceTest_sparse_binary_values_variances_size_fail
    : public ::testing::Test {
protected:
  Dimensions dims{{Dim::Y, Dim::X}, {2, Dimensions::Sparse}};
  Variable a = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(2)},
      {sparse_container<double>(2), sparse_container<double>(2)});
  Variable val_var = a;
  Variable val = makeVariable<double>(
      dims, {sparse_container<double>(2), sparse_container<double>(2)});
  static constexpr auto op = [](auto &a, const auto b) { a *= b; };
};

TEST_F(TransformInPlaceTest_sparse_binary_values_variances_size_fail,
       baseline) {
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op));
  ASSERT_NO_THROW(transform_in_place<pair_self_t<double>>(a, val, op));
};

TEST_F(TransformInPlaceTest_sparse_binary_values_variances_size_fail,
       a_values_size_bad) {
  a.sparseValues<double>()[1].resize(1);
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op));
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val, op));
};

TEST_F(TransformInPlaceTest_sparse_binary_values_variances_size_fail,
       a_variances_size_bad) {
  a.sparseVariances<double>()[1].resize(1);
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op));
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val, op));
};

TEST_F(TransformInPlaceTest_sparse_binary_values_variances_size_fail,
       val_var_values_size_bad) {
  val_var.sparseValues<double>()[1].resize(1);
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op));
};

TEST_F(TransformInPlaceTest_sparse_binary_values_variances_size_fail,
       val_var_variances_size_bad) {
  val_var.sparseVariances<double>()[1].resize(1);
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val_var, op));
};

TEST_F(TransformInPlaceTest_sparse_binary_values_variances_size_fail,
       val_values_size_bad) {
  val.sparseValues<double>()[1].resize(1);
  ASSERT_ANY_THROW(transform_in_place<pair_self_t<double>>(a, val, op));
};

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

TEST(TransformInPlaceTest, sparse_val_var_with_sparse_val_var) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = make_sparse_variable_with_variance();
  set_sparse_values(b, {{0.1, 0.2, 0.3}, {0.4}});
  set_sparse_variances(b, {{0.5, 0.6, 0.7}, {0.8}});

  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &a, const auto b) { a *= b; });

  auto expected = make_sparse_variable();
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
}

TEST(TransformInPlaceTest, sparse_val_var_with_sparse_val) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = make_sparse_variable();
  set_sparse_values(b, {{0.1, 0.2, 0.3}, {0.4}});

  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &a, const auto b) { a *= b; });

  auto expected = make_sparse_variable();
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
}

TEST(TransformInPlaceTest, sparse_val_var_with_val_var) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = makeVariable<double>({Dim::Y, 2}, {1.5, 1.6}, {1.7, 1.8});

  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &a, const auto b) { a *= b; });

  auto expected = make_sparse_variable();
  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<double>({1.5}, {1.7});
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<double>({1.6}, {1.8});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(equals(a.sparseValues<double>()[0], expected0.values<double>()));
  EXPECT_TRUE(equals(a.sparseValues<double>()[1], expected1.values<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[0], expected0.variances<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[1], expected1.variances<double>()));
}

TEST(TransformInPlaceTest, sparse_val_var_with_val) {
  auto a = make_sparse_variable_with_variance();
  set_sparse_values(a, {{1, 2, 3}, {4}});
  set_sparse_variances(a, {{5, 6, 7}, {8}});
  auto b = makeVariable<double>({Dim::Y, 2}, {1.5, 1.6});

  transform_in_place<pair_self_t<double>>(
      a, b, [](auto &a, const auto b) { a *= b; });

  auto expected = make_sparse_variable();
  auto a0 = makeVariable<double>({Dim::X, 3}, {1, 2, 3}, {5, 6, 7});
  auto b0 = makeVariable<double>({1.5});
  auto a1 = makeVariable<double>({Dim::X, 1}, {4}, {8});
  auto b1 = makeVariable<double>({1.6});
  auto expected0 = a0 * b0;
  auto expected1 = a1 * b1;
  EXPECT_TRUE(equals(a.sparseValues<double>()[0], expected0.values<double>()));
  EXPECT_TRUE(equals(a.sparseValues<double>()[1], expected1.values<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[0], expected0.variances<double>()));
  EXPECT_TRUE(
      equals(a.sparseVariances<double>()[1], expected1.variances<double>()));
}
