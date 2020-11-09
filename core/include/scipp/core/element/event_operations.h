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

constexpr auto get = [](const auto &x, const scipp::index i) {
  if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
    return ValueAndVariance{x.value[i], x.variance[i]};
  else
    return x[i];
};

namespace map_in_place_detail {
template <class Coord, class Edge, class Weight>
using args = std::tuple<Weight, Coord, span<const Edge>, span<const Weight>>;
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
    }};

constexpr auto map_in_place_linspace =
    overloaded{map_in_place, [](auto &out, const auto &coord, const auto &edges,
                                const auto &weights) {
                 const auto [offset, nbin, factor] = linear_edge_params(edges);
                 const auto bin = (coord - offset) * factor;
                 if (bin < 0.0 || bin >= nbin)
                   out = 0.0;
                 else
                   out = get(weights, bin);
               }};

constexpr auto map_in_place_sorted_edges =
    overloaded{map_in_place, [](auto &out, const auto &coord, const auto &edges,
                                const auto &weights) {
                 auto it = std::upper_bound(edges.begin(), edges.end(), coord);
                 if (it == edges.end() || it == edges.begin())
                   out = 0.0;
                 else
                   out = get(weights, --it - edges.begin());
               }};

namespace map_and_mul_detail {
template <class Data, class Coord, class Edge, class Weight>
using args = std::tuple<Data, Coord, span<const Edge>, span<const Weight>>;
} // namespace map_and_mul_detail

constexpr auto map_and_mul = overloaded{
    element::arg_list<map_and_mul_detail::args<double, double, double, double>,
                      map_and_mul_detail::args<float, double, double, double>,
                      map_and_mul_detail::args<float, double, double, float>,
                      map_and_mul_detail::args<double, float, float, double>>,
    transform_flags::expect_in_variance_if_out_variance,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>,
    [](units::Unit &data, const units::Unit &x, const units::Unit &edges,
       const units::Unit &weights) {
      expect::equals(x, edges);
      data *= weights;
    }};

constexpr auto map_and_mul_linspace =
    overloaded{map_and_mul, [](auto &data, const auto coord, const auto &edges,
                               const auto &weights) {
                 const auto [offset, nbin, factor] = linear_edge_params(edges);
                 const auto bin = (coord - offset) * factor;
                 if (bin < 0.0 || bin >= nbin)
                   data *= 0.0;
                 else
                   data *= get(weights, bin);
               }};

constexpr auto map_and_mul_sorted_edges =
    overloaded{map_and_mul, [](auto &data, const auto coord, const auto &edges,
                               const auto &weights) {
                 auto it = std::upper_bound(edges.begin(), edges.end(), coord);
                 if (it == edges.end() || it == edges.begin())
                   data *= 0.0;
                 else
                   data *= get(weights, --it - edges.begin());
               }};

} // namespace scipp::core::element::event
