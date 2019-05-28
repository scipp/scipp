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
  transform_in_place<double>(var, [](const double x) { return -x; });
  EXPECT_TRUE(equals(var.values<double>(), {-1.1, -2.2}));
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
      b, a, [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.values<double>(), {4.4, 5.5}));
}

TEST(TransformTest, apply_binary_in_place_var_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});
  transform_in_place<pair_self_t<double>>(
      b(Dim::Y, 1), a, [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.values<double>(), {4.4, 5.5}));
}

TEST(TransformTest, apply_binary_in_place_view_with_var) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>(3.3);
  transform_in_place<pair_self_t<double>>(
      b, a(Dim::X, 1), [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.values<double>(), {1.1, 5.5}));
}

TEST(TransformTest, apply_binary_in_place_view_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});
  transform_in_place<pair_self_t<double>>(
      b(Dim::Y, 1), a(Dim::X, 1),
      [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.values<double>(), {1.1, 5.5}));
}

TEST(TransformTest, transform_combines_uncertainty_propgation) {
  auto a = makeVariable<double>({Dim::X, 1}, {2.0}, {0.1});
  const auto b = makeVariable<double>(3.0, 0.2);
  transform_in_place<pair_self_t<double>>(
      b, a, [](const auto x, const auto y) { return x * y + y; });
  EXPECT_TRUE(equals(a.values<double>(), {2.0 * 3.0 + 3.0}));
  EXPECT_TRUE(equals(a.variances<double>(), {0.1 * 3 * 3 + 0.2 * 2 * 2 + 0.2}));
}

TEST(TransformTest, unary_on_elements_of_sparse) {
  auto a = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<double>(a, [](const double x) { return sqrt(x); });
  EXPECT_TRUE(equals(a_[0], {1, 2, 3}));
  EXPECT_TRUE(equals(a_[1], {2}));
}

TEST(TransformTest, unary_on_sparse_container) {
  auto a = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<sparse_container<double>>(
      a, [](const auto &x) { return decltype(x){}; });
  EXPECT_TRUE(a_[0].empty());
  EXPECT_TRUE(a_[1].empty());
}

TEST(TransformTest, binary_with_dense) {
  auto sparse = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto sparse_ = sparse.sparseSpan<double>();
  sparse_[0] = {1, 2, 3};
  sparse_[1] = {4};
  auto dense = makeVariable<double>({Dim::Y, 2}, {1.5, 0.5});

  transform_in_place<pair_self_t<double>>(
      dense, sparse, [](const auto a, const auto b) { return a * b; });

  EXPECT_TRUE(equals(sparse_[0], {1.5, 3.0, 4.5}));
  EXPECT_TRUE(equals(sparse_[1], {2.0}));
}
