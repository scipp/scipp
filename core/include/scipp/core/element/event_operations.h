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

namespace copy_if_detail {
template <class T, class Index>
using args = std::tuple<event_list<T>, event_list<Index>>;

} // namespace copy_if_detail

constexpr auto copy_if =
    overloaded{element::arg_list<copy_if_detail::args<double, int32_t>,
                                 copy_if_detail::args<float, int32_t>,
                                 copy_if_detail::args<int64_t, int32_t>,
                                 copy_if_detail::args<int32_t, int32_t>,
                                 copy_if_detail::args<double, int64_t>,
                                 copy_if_detail::args<float, int64_t>,
                                 copy_if_detail::args<int64_t, int64_t>,
                                 copy_if_detail::args<int32_t, int64_t>>,
               transform_flags::expect_no_variance_arg<1>,
               [](const units::Unit &var, const units::Unit &select) {
                 expect::equals(select, units::one);
                 return var;
               },
               [](const auto &var, const auto &select) {
                 using VarT = std::decay_t<decltype(var)>;
                 using Events = event_list<typename VarT::value_type>;
                 const auto size = scipp::size(select);
                 if constexpr (is_ValuesAndVariances_v<VarT>) {
                   std::pair<Events, Events> out;
                   out.first.reserve(size);
                   out.second.reserve(size);
                   for (const auto i : select) {
                     out.first.push_back(var.values[i]);
                     out.second.push_back(var.variances[i]);
                   }
                   return out;
                 } else {
                   Events out;
                   out.reserve(size);
                   for (const auto i : select)
                     out.push_back(var[i]);
                   return out;
                 }
               }};

namespace map_detail {
template <class Coord, class Edge, class Weight>
using args =
    std::tuple<event_list<Coord>, span<const Edge>, span<const Weight>>;
} // namespace map_detail

constexpr auto map = overloaded{
    element::arg_list<map_detail::args<int64_t, int64_t, double>,
                      map_detail::args<int64_t, int64_t, float>,
                      map_detail::args<int32_t, int32_t, double>,
                      map_detail::args<int32_t, int32_t, float>,
                      map_detail::args<int64_t, double, double>,
                      map_detail::args<int64_t, double, float>,
                      map_detail::args<int32_t, double, double>,
                      map_detail::args<int32_t, double, float>,
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
      using T = event_list<ElemT>;
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
      using w_type = decltype(get(weights, 0));
      constexpr w_type out_of_bounds(0.0);
      if (scipp::numeric::is_linspace(edges)) {
        const auto [offset, nbin, scale] = linear_edge_params(edges);
        for (const auto c : coord) {
          const auto bin = (c - offset) * scale;
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
        for (const auto c : coord) {
          auto it = std::upper_bound(edges.begin(), edges.end(), c);
          w_type w = (it == edges.end() || it == edges.begin())
                         ? out_of_bounds
                         : get(weights, --it - edges.begin());
          if constexpr (vars) {
            out_vals.emplace_back(w.value);
            out_vars.emplace_back(w.variance);
          } else {
            out_vals.emplace_back(w);
          }
        }
      }
      if constexpr (vars)
        return std::pair(std::move(out_vals), std::move(out_vars));
      else
        return out_vals;
    }};

namespace make_select_detail {
template <class T> using args = std::tuple<event_list<T>, span<const T>>;
}

template <class T>
constexpr auto make_select = overloaded{
    element::arg_list<
        make_select_detail::args<double>, make_select_detail::args<float>,
        make_select_detail::args<int64_t>, make_select_detail::args<int32_t>>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const units::Unit &coord, const units::Unit &interval) {
      expect::equals(coord, interval);
      return units::one;
    },
    [](const auto &coord, const auto &interval) {
      const auto low = interval[0];
      const auto high = interval[1];
      const auto size = scipp::size(coord);
      event_list<T> select;
      for (scipp::index i = 0; i < size; ++i)
        if (coord[i] >= low && coord[i] < high)
          select.push_back(i);
      return select;
    }};

} // namespace scipp::core::element::event
