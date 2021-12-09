// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"

#include "scipp/units/unit.h"

#include "scipp/core/element/arg_list.h"
#include "scipp/core/histogram.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"

namespace scipp::core::element::event {

constexpr auto get = [](const auto &x, const scipp::index i) {
  if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
    return ValueAndVariance{x.value[i], x.variance[i]};
  else
    return x[i];
};

namespace map_detail {
template <class Coord, class Edge, class Weight>
using args =
    std::tuple<Coord, scipp::span<const Edge>, scipp::span<const Weight>>;
} // namespace map_detail

constexpr auto map = overloaded{
    element::arg_list<map_detail::args<int64_t, int64_t, double>,
                      map_detail::args<int64_t, int64_t, float>,
                      map_detail::args<int64_t, int64_t, int64_t>,
                      map_detail::args<int64_t, int64_t, int32_t>,
                      map_detail::args<int64_t, int64_t, bool>,
                      map_detail::args<int32_t, int32_t, double>,
                      map_detail::args<int32_t, int32_t, float>,
                      map_detail::args<int32_t, int32_t, int64_t>,
                      map_detail::args<int32_t, int32_t, int32_t>,
                      map_detail::args<int32_t, int32_t, bool>,
                      map_detail::args<int64_t, double, double>,
                      map_detail::args<int64_t, double, float>,
                      map_detail::args<int64_t, double, int64_t>,
                      map_detail::args<int64_t, double, int32_t>,
                      map_detail::args<int64_t, double, bool>,
                      map_detail::args<int32_t, double, double>,
                      map_detail::args<int32_t, double, float>,
                      map_detail::args<int32_t, double, int64_t>,
                      map_detail::args<int32_t, double, int32_t>,
                      map_detail::args<int32_t, double, bool>,
                      map_detail::args<time_point, time_point, double>,
                      map_detail::args<time_point, time_point, float>,
                      map_detail::args<time_point, time_point, int64_t>,
                      map_detail::args<time_point, time_point, int32_t>,
                      map_detail::args<time_point, time_point, bool>,
                      map_detail::args<double, double, double>,
                      map_detail::args<double, double, float>,
                      map_detail::args<double, double, int64_t>,
                      map_detail::args<double, double, int32_t>,
                      map_detail::args<double, double, bool>,
                      map_detail::args<float, double, double>,
                      map_detail::args<float, double, float>,
                      map_detail::args<float, double, int64_t>,
                      map_detail::args<float, double, int32_t>,
                      map_detail::args<float, double, bool>,
                      map_detail::args<float, float, float>,
                      map_detail::args<float, float, int64_t>,
                      map_detail::args<float, float, int32_t>,
                      map_detail::args<float, float, bool>,
                      map_detail::args<double, float, double>,
                      map_detail::args<double, float, float>,
                      map_detail::args<double, float, int64_t>,
                      map_detail::args<double, float, int32_t>,
                      map_detail::args<double, float, bool>>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const units::Unit &x, const units::Unit &edges,
       const units::Unit &weights) {
      expect::equals(x, edges);
      return weights;
    }};

constexpr auto map_linspace = overloaded{
    map, [](const auto &coord, const auto &edges, const auto &weights) {
      const auto [offset, nbin, factor] = linear_edge_params(edges);
      const auto bin = (coord - offset) * factor;
      using T = std::decay_t<decltype(get(weights, 0))>;
      return (bin < 0.0 || bin >= nbin) ? T{0} : get(weights, bin);
    }};

constexpr auto map_sorted_edges = overloaded{
    map, [](const auto &coord, const auto &edges, const auto &weights) {
      auto it = std::upper_bound(edges.begin(), edges.end(), coord);
      using T = std::decay_t<decltype(get(weights, 0))>;
      return (it == edges.end() || it == edges.begin())
                 ? T{0}
                 : get(weights, --it - edges.begin());
    }};

namespace map_and_mul_detail {
template <class Data, class Coord, class Edge, class Weight>
using args =
    std::tuple<Data, Coord, scipp::span<const Edge>, scipp::span<const Weight>>;
} // namespace map_and_mul_detail

constexpr auto map_and_mul = overloaded{
    element::arg_list<
        map_and_mul_detail::args<double, double, double, double>,
        map_and_mul_detail::args<double, double, double, float>,
        map_and_mul_detail::args<float, double, double, double>,
        map_and_mul_detail::args<float, double, double, float>,
        map_and_mul_detail::args<double, float, float, double>,
        map_and_mul_detail::args<double, time_point, time_point, double>,
        map_and_mul_detail::args<double, time_point, time_point, float>,
        map_and_mul_detail::args<float, time_point, time_point, double>,
        map_and_mul_detail::args<float, time_point, time_point, float>>,
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
