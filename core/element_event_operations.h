// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"

#include "scipp/core/histogram.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"

#include "arg_list.h"

namespace scipp::core::element::event {

namespace map_detail {
template <class Coord, class Edge, class Weight>
using args =
    std::tuple<event_list<Coord>, span<const Edge>, span<const Weight>>;
} // namespace map_detail

constexpr auto map = overloaded{
    element::arg_list<map_detail::args<int64_t, double, double>,
                      map_detail::args<double, double, double>,
                      map_detail::args<float, double, double>,
                      map_detail::args<float, float, float>,
                      map_detail::args<double, float, float>>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const units::Unit &x, const units::Unit &edges,
       const units::Unit &weights) {
      expect::equals(x, edges);
      return weights;
    },
    [](const auto &coord, const auto &edges, const auto &weights) {
      using W = std::decay_t<decltype(weights)>;
      constexpr bool vars = is_ValueAndVariance_v<W>;
      using ElemT = typename core::detail::element_type_t<W>::value_type;
      using T = sparse_container<ElemT>;
      T out_vals;
      T out_vars;
      out_vals.reserve(coord.size());
      if constexpr (vars)
        out_vars.reserve(coord.size());
      constexpr auto get = [](const auto &x, const scipp::index i) {
        if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
          return ValueAndVariance{x.value[i], x.variance[i]};
        else
          return x[i];
      };
      if (scipp::numeric::is_linspace(edges)) {
        const auto [offset, nbin, scale] = linear_edge_params(edges);
        for (const auto c : coord) {
          const auto bin = (c - offset) * scale;
          using w_type = decltype(get(weights, bin));
          constexpr w_type out_of_bounds(0.0);
          w_type w =
              bin < 0.0 || bin >= nbin ? out_of_bounds : get(weights, bin);
          if constexpr (vars) {
            out_vals.emplace_back(w.value);
            out_vars.emplace_back(w.variance);
          } else {
            out_vals.emplace_back(w);
          }
        }
      } else {
        expect::histogram::sorted_edges(edges);
        throw std::runtime_error("Non-constant bin width not supported yet.");
      }
      if constexpr (vars)
        return std::pair(std::move(out_vals), std::move(out_vars));
      else
        return out_vals;
    }};

} // namespace scipp::core::element::event
