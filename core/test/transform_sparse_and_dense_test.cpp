// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

#include "../operators.h"

using namespace scipp;
using namespace scipp::core;

TEST(TransformSparseAndDenseTest, two_args) {
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X},
                                    Shape{2l, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  auto dense = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      Values{1.1, 2.2, 3.3, 4.4});
  auto dense_view = subspan_view(dense, Dim::X);

  const auto result = transform<
      pair_custom_t<std::pair<sparse_container<double>, span<double>>>>(
      var, dense_view,
      overloaded{[](const auto &a, const auto &b) {
                   EXPECT_EQ(b.size(), 2);
                   return a;
                 },
                 [](const units::Unit &a, const units::Unit &) { return a; },
                 transform_flags::expect_no_variance_arg<0>,
                 transform_flags::expect_no_variance_arg<1>});
  EXPECT_EQ(result, var);
}

TEST(TransformSparseAndDenseTest, three_args) {
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X},
                                    Shape{2l, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  auto dense = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                      Values{1.1, 2.2, 3.3, 4.4});
  auto dense_view = subspan_view(dense, Dim::X);

  auto dense_with_variance = createVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{0.1, 0.2}, Variances{0.3, 0.4});
  auto dense_with_variance_view = subspan_view(dense_with_variance, Dim::X);

  const auto out = transform<std::tuple<
      std::tuple<sparse_container<double>, span<double>, span<double>>>>(
      var, dense_view, dense_with_variance_view,
      overloaded{
          [](const auto &a, const auto &, const auto &c) {
            if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(c)>>) {
              return std::pair(a, a);
            } else
              return a;
          },
          transform_flags::expect_no_variance_arg<0>,
          transform_flags::expect_no_variance_arg<1>,
          [](const units::Unit &a, const units::Unit &, const units::Unit &) {
            return a;
          }});
  EXPECT_TRUE(out.hasVariances());
  EXPECT_TRUE(equals(out.sparseValues<double>(), var.sparseValues<double>()));
  EXPECT_TRUE(
      equals(out.sparseVariances<double>(), var.sparseValues<double>()));
}
