// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
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
  std::vector<span<double>> spans;
  for (scipp::index i = 0; i < 2; ++i)
    spans.emplace_back(dense.values<double>().subspan(2 * i, 2));
  auto dense_view = makeVariable<span<double>>({Dim::Y, 2}, spans);

  const auto result = transform<
      pair_custom_t<std::pair<sparse_container<double>, span<double>>>>(
      var, dense_view,
      overloaded{[](const auto &a, const auto &b) {
                   fprintf(stderr, "%ld %ld\n", a.size(), b.size());
                   for (auto &a_ : a)
                     fprintf(stderr, "a: %lf\n", a_);
                   for (auto &b_ : b)
                     fprintf(stderr, "b: %lf\n", b_);
                   return a;
                 },
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
  std::vector<span<double>> spans;
  for (scipp::index i = 0; i < 2; ++i)
    spans.emplace_back(dense.values<double>().subspan(2 * i, 2));
  auto dense_view = makeVariable<span<double>>({Dim::Y, 2}, spans);

  auto dense_with_variance =
      makeVariable<double>({Dim::X, 2}, {0.1, 0.2}, {0.3, 0.4});
  std::vector<span<double>> spans_val{dense_with_variance.values<double>()};
  std::vector<span<double>> spans_var{dense_with_variance.variances<double>()};
  auto dense_with_variance_view =
      makeVariable<span<double>>({}, spans_val, spans_var);

  const auto out = transform<std::tuple<
      std::tuple<sparse_container<double>, span<double>, span<double>>>>(
      var, dense_view, dense_with_variance_view,
      overloaded{[](const auto &a, const auto &, const auto &c) {
                   if constexpr (core::detail::is_ValueAndVariance_v<
                                     std::decay_t<decltype(c)>>) {
                     return std::pair(a, a);
                   } else
                     return a;
                 },
                 [](const units::Unit &a, const units::Unit &,
                    const units::Unit &) { return a; }});
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

  auto edges =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {0, 2, 4, 1, 3, 5});
  std::vector<span<double>> spans;
  for (scipp::index i = 0; i < 2; ++i)
    spans.emplace_back(edges.values<double>().subspan(3 * i, 3));
  auto edges_view = makeVariable<span<double>>({Dim::Y, 2}, spans);

  auto weights = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});
  std::vector<span<double>> spans_val{weights.values<double>()};
  std::vector<span<double>> spans_var{weights.variances<double>()};
  auto weights_view = makeVariable<span<double>>({}, spans_val, spans_var);

  DataArray hist(weights, {{Dim::X, edges}});

  const auto out = transform<std::tuple<
      std::tuple<sparse_container<double>, span<double>, span<double>>>>(
      var, edges_view, weights_view,
      overloaded{
          [](const auto &sparse, const auto &edges, const auto &weights) {
            expect::histogram::sorted_edges(edges);
            constexpr bool have_variance = core::detail::is_ValueAndVariance_v<
                std::decay_t<decltype(weights)>>;
            using T =
                sparse_container<float>; // TODO use type of input weights?
            T out_vals;
            T out_vars;
            out_vals.reserve(sparse.size());
            if (have_variance)
              out_vars.reserve(sparse.size());
            if (scipp::numeric::is_linspace(edges)) {
              auto len = scipp::size(sparse) - 1;
              const double offset = edges.front();
              const double nbin = static_cast<double>(len);
              const double scale = nbin / (edges.back() - edges.front());
              for (const auto c : sparse) {
                const double bin = (c - offset) * scale;
                if (bin >= 0.0 && bin < nbin) {
                  if constexpr (have_variance) {
                    out_vals.emplace_back(weights.value[bin]);
                    out_vars.emplace_back(weights.variance[bin]);
                  } else {
                    out_vals.emplace_back(weights[bin]);
                  }
                } else {
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
          [](const units::Unit &, const units::Unit &, const units::Unit &c) {
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
