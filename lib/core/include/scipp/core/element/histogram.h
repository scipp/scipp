// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <numeric>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

namespace {
constexpr auto iadd = [](const auto &x1, const scipp::index i1, const auto &x2,
                         const scipp::index i2) {
  using V = std::decay_t<decltype(x1)>;
  if constexpr (is_ValueAndVariance_v<V>) {
    x1.value[i1] += x2.value[i2];
    x1.variance[i1] += x2.variance[i2];
  } else {
    x1[i1] += x2[i2];
  }
};
} // namespace

namespace histogram_detail {
template <class Coord, class Weight>
using args = std::tuple<std::span<Weight>, std::span<const Coord>,
                        std::span<const Weight>, std::span<const Coord>>;
}

static constexpr auto histogram = overloaded{
    element::arg_list<histogram_detail::args<double, double>,
                      histogram_detail::args<double, float>,
                      histogram_detail::args<double, int64_t>,
                      histogram_detail::args<double, int32_t>,
                      histogram_detail::args<float, double>,
                      histogram_detail::args<float, float>,
                      histogram_detail::args<float, int64_t>,
                      histogram_detail::args<float, int32_t>,
                      histogram_detail::args<int32_t, double>,
                      histogram_detail::args<int32_t, float>,
                      histogram_detail::args<int32_t, int64_t>,
                      histogram_detail::args<int32_t, int32_t>,
                      histogram_detail::args<int64_t, double>,
                      histogram_detail::args<int64_t, float>,
                      histogram_detail::args<int64_t, int64_t>,
                      histogram_detail::args<int64_t, int32_t>,
                      histogram_detail::args<time_point, double>,
                      histogram_detail::args<time_point, float>,
                      histogram_detail::args<time_point, int64_t>,
                      histogram_detail::args<time_point, int32_t>>,
    [](const auto &data, const auto &events, const auto &weights,
       const auto &edges) {
      zero(data);
      // Special implementation for linear bins. Gives a 1x to 20x speedup
      // for few and many events per histogram, respectively.
      if (scipp::numeric::islinspace(edges)) {
        const auto params = core::linear_edge_params(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          if (const auto bin = get_bin<scipp::index>(x, edges, params);
              bin >= 0)
            iadd(data, bin, weights, i);
        }
      } else {
        core::expect::histogram::sorted_edges(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          auto it = std::upper_bound(edges.begin(), edges.end(), x);
          if (it != edges.end() && it != edges.begin())
            iadd(data, --it - edges.begin(), weights, i);
        }
      }
    },
    [](const sc_units::Unit &events_unit, const sc_units::Unit &weights_unit,
       const sc_units::Unit &edge_unit) {
      if (events_unit != edge_unit)
        throw except::UnitError(
            "Bin edges must have same unit as the input coordinate.");
      return weights_unit;
    },
    transform_flags::expect_in_variance_if_out_variance,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<3>};

} // namespace scipp::core::element
