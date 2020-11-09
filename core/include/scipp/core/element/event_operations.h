// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"

#include "scipp/units/unit.h"

#include "scipp/core/element/arg_list.h"
#include "scipp/core/histogram.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"

namespace scipp::core::element::event {

namespace map_in_place_detail {
template <class Coord, class Edge, class Weight>
using args = std::tuple<span<Weight>, span<const Coord>, span<const Edge>,
                        span<const Weight>>;
} // namespace map_in_place_detail

constexpr auto map_in_place = overloaded{
    element::arg_list<map_in_place_detail::args<int64_t, int64_t, double>,
                      map_in_place_detail::args<int64_t, int64_t, float>,
                      map_in_place_detail::args<int32_t, int32_t, double>,
                      map_in_place_detail::args<int32_t, int32_t, float>,
                      map_in_place_detail::args<int64_t, double, double>,
                      map_in_place_detail::args<int64_t, double, float>,
                      map_in_place_detail::args<int32_t, double, double>,
                      map_in_place_detail::args<int32_t, double, float>,
                      map_in_place_detail::args<double, double, double>,
                      map_in_place_detail::args<double, double, float>,
                      map_in_place_detail::args<float, double, double>,
                      map_in_place_detail::args<float, float, float>,
                      map_in_place_detail::args<double, float, float>>,
    transform_flags::expect_in_variance_if_out_variance,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>,
    [](units::Unit &out, const units::Unit &x, const units::Unit &edges,
       const units::Unit &weights) {
      expect::equals(x, edges);
      out = weights;
    },
    [](const auto &out, const auto &coord, const auto &edges,
       const auto &weights) {
      using W = std::decay_t<decltype(weights)>;
      constexpr bool vars = is_ValueAndVariance_v<W>;
      constexpr auto get = [](const auto &x, const scipp::index i) {
        if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
          return ValueAndVariance{x.value[i], x.variance[i]};
        else
          return x[i];
      };
      using w_type = decltype(get(weights, 0));
      constexpr w_type out_of_bounds(0.0);
      if (scipp::numeric::is_linspace(edges)) {
        const auto [offset, nbin, scale] = linear_edge_params(edges);
        for (scipp::index i = 0; i < scipp::size(coord); ++i) {
          const auto bin = (coord[i] - offset) * scale;
          w_type w =
              bin < 0.0 || bin >= nbin ? out_of_bounds : get(weights, bin);
          if constexpr (vars) {
            out.value[i] = w.value;
            out.variance[i] = w.variance;
          } else {
            out[i] = w;
          }
        }
      } else {
        expect::histogram::sorted_edges(edges);
        for (scipp::index i = 0; i < scipp::size(coord); ++i) {
          auto it = std::upper_bound(edges.begin(), edges.end(), coord[i]);
          w_type w = (it == edges.end() || it == edges.begin())
                         ? out_of_bounds
                         : get(weights, --it - edges.begin());
          if constexpr (vars) {
            out.value[i] = w.value;
            out.variance[i] = w.variance;
          } else {
            out[i] = w;
          }
        }
      }
    }};

} // namespace scipp::core::element::event
