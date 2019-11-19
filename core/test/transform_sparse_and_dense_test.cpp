// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

#include "../histogram.h"
#include "../operators.h"

using namespace scipp;
using namespace scipp::core;

TEST(TransformSparseAndDenseTest, two_args) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  auto dense =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.1, 2.2, 3.3, 4.4});
  auto dense_view = subspan_view(dense, Dim::X);

  const auto result = transform<
      pair_custom_t<std::pair<sparse_container<double>, span<double>>>>(
      var, dense_view,
      overloaded{[](const auto &a, const auto &) { return a; },
                 [](const units::Unit &a, const units::Unit &) { return a; },
                 transform_flags::expect_no_variance_arg<0>,
                 transform_flags::expect_no_variance_arg<1>});
}

TEST(TransformSparseAndDenseTest, three_args) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  auto dense =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.1, 2.2, 3.3, 4.4});
  auto dense_view = subspan_view(dense, Dim::X);

  auto dense_with_variance =
      makeVariable<double>({Dim::X, 2}, {0.1, 0.2}, {0.3, 0.4});
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

TEST(TransformSparseAndDenseTest, sparse_times_dense) {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto vals = var.sparseValues<double>();
  vals[0] = {1.1, 2.2, 3.3};
  vals[1] = {1.1, 2.2, 3.3, 5.5};

  auto edges_ =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {0, 2, 4, 1, 3, 5});
  auto edges_view = subspan_view(edges_, Dim::X);

  auto weights_ = makeVariable<float>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});
  auto weights_view = subspan_view(weights_, Dim::X);

  DataArray hist(weights_, {{Dim::X, edges_}});

  const auto out = transform<std::tuple<
      std::tuple<sparse_container<double>, span<double>, span<float>>>>(
      var, edges_view, weights_view,
      overloaded{
          [](const auto &sparse, const auto &edges, const auto &weights) {
            expect::histogram::sorted_edges(edges);
            using W = std::decay_t<decltype(weights)>;
            constexpr bool have_variance = is_ValueAndVariance_v<W>;
            using T = sparse_container<
                typename core::detail::element_type_t<W>::value_type>;
            T out_vals;
            T out_vars;
            out_vals.reserve(sparse.size());
            if (have_variance)
              out_vars.reserve(sparse.size());
            if (scipp::numeric::is_linspace(edges)) {
              auto len = scipp::size(sparse) - 1;
              const auto offset = edges.front();
              const auto nbin = static_cast<decltype(offset)>(len);
              const auto scale = nbin / (edges.back() - edges.front());
              for (const auto c : sparse) {
                const auto bin = (c - offset) * scale;
                if (bin >= 0.0 && bin < nbin) {
                  if constexpr (have_variance) {
                    out_vals.emplace_back(weights.value[bin]);
                    out_vars.emplace_back(weights.variance[bin]);
                  } else {
                    out_vals.emplace_back(weights[bin]);
                  }
                } else {
                  // TODO should we set NAN instead?
                  out_vals.emplace_back(1.0);
                  if constexpr (have_variance)
                    out_vars.emplace_back(1.0);
                }
              }
            } else {
              throw std::runtime_error("Only equal-sized bins supported.");
            }
            if constexpr (have_variance) {
              return std::pair(std::move(out_vals), std::move(out_vars));
            } else {
              return out_vals;
            }
          },
          transform_flags::expect_no_variance_arg<0>,
          transform_flags::expect_no_variance_arg<1>,
          [](const units::Unit &a, const units::Unit &b, const units::Unit &c) {
            expect::equals(a, b);
            return c;
          }});
  EXPECT_TRUE(out.hasVariances());
  const auto out_vals = out.sparseValues<float>();
  EXPECT_TRUE(equals(out_vals[0], {2, 3, 3}));
  EXPECT_TRUE(equals(out_vals[1], {2, 2, 3, 1}));
  const auto out_vars = out.sparseVariances<float>();
  EXPECT_TRUE(equals(out_vars[0], {0.3f, 0.4f, 0.4f}));
  EXPECT_TRUE(equals(out_vars[1], {0.3f, 0.3f, 0.4f, 1.0f}));
}
