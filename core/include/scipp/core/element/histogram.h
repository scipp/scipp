// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

namespace {
constexpr auto value = [](const auto &v, const scipp::index idx) {
  using V = std::decay_t<decltype(v)>;
  if constexpr (is_ValueAndVariance_v<V>) {
    if constexpr (std::is_arithmetic_v<typename V::value_type>) {
      static_cast<void>(idx);
      return v.value;
    } else {
      return v.value[idx];
    }
  } else
    return v.values[idx];
};
constexpr auto variance = [](const auto &v, const scipp::index idx) {
  using V = std::decay_t<decltype(v)>;
  if constexpr (is_ValueAndVariance_v<V>) {
    if constexpr (std::is_arithmetic_v<typename V::value_type>) {
      static_cast<void>(idx);
      return v.variance;
    } else {
      return v.variance[idx];
    }
  } else
    return v.variances[idx];
};
} // namespace

namespace histogram_detail {
template <class Out, class Coord, class Weight, class Edge>
using args = std::tuple<span<Out>, span<const Coord>, span<const Weight>,
                        span<const Edge>>;
}

static constexpr auto histogram = overloaded{
    transform_flags::zero_output,
    element::arg_list<histogram_detail::args<float, double, float, double>,
                      histogram_detail::args<double, double, double, double>,
                      histogram_detail::args<double, float, double, double>,
                      histogram_detail::args<double, float, double, float>,
                      histogram_detail::args<double, double, float, double>>,
    [](const auto &data, const auto &events, const auto &weights,
       const auto &edges) {
      // Special implementation for linear bins. Gives a 1x to 20x speedup
      // for few and many events per histogram, respectively.
      if (scipp::numeric::is_linspace(edges)) {
        const auto [offset, nbin, scale] = core::linear_edge_params(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          const double bin = (x - offset) * scale;
          if (bin >= 0.0 && bin < nbin) {
            const auto b = static_cast<scipp::index>(bin);
            const auto w = value(weights, i);
            const auto e = variance(weights, i);
            data.value[b] += w;
            data.variance[b] += e;
          }
        }
      } else {
        core::expect::histogram::sorted_edges(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          auto it = std::upper_bound(edges.begin(), edges.end(), x);
          if (it != edges.end() && it != edges.begin()) {
            const auto b = --it - edges.begin();
            const auto w = value(weights, i);
            const auto e = variance(weights, i);
            data.value[b] += w;
            data.variance[b] += e;
          }
        }
      }
    },
    [](const units::Unit &events_unit, const units::Unit &weights_unit,
       const units::Unit &edge_unit) {
      if (events_unit != edge_unit)
        throw except::UnitError("Bin edges must have same unit as the events "
                                "input coordinate.");
      if (weights_unit != units::counts && weights_unit != units::dimensionless)
        throw except::UnitError("Weights of event data must be "
                                "`units::counts` or `units::dimensionless`.");
      return weights_unit;
    },
    transform_flags::expect_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_variance_arg<2>,
    transform_flags::expect_no_variance_arg<3>};

template <class T> using bin_index_arg = std::tuple<T, span<const T>>;

static constexpr auto bin_index =
    overloaded{element::arg_list<bin_index_arg<double>, bin_index_arg<float>>,
               [](const units::Unit &coord, const units::Unit &edges) {
                 expect::equals(coord, edges);
                 return units::one;
               },
               transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>};

// Special faster implementation for linear bins.
static constexpr auto bin_index_linspace =
    overloaded{bin_index, [](const auto &x, const auto &edges) -> scipp::index {
                 const auto [offset, nbin, scale] =
                     core::linear_edge_params(edges);
                 const double bin = (x - offset) * scale;
                 return (bin < 0.0 || bin >= nbin) ? -1 : bin;
               }};

static constexpr auto bin_index_sorted_edges =
    overloaded{bin_index, [](const auto &x, const auto &edges) -> scipp::index {
                 auto it = std::upper_bound(edges.begin(), edges.end(), x);
                 return (it == edges.begin() || it == edges.end())
                            ? -1
                            : --it - edges.begin();
               }};

static constexpr auto groups_to_map = overloaded{
    element::arg_list<span<const int64_t>, span<const int32_t>,
                      span<const std::string>>,
    transform_flags::expect_no_variance_arg<0>,
    [](const units::Unit &u) { return u; },
    [](const auto &groups) {
      std::unordered_map<typename std::decay_t<decltype(groups)>::value_type,
                         scipp::index>
          index;
      scipp::index current = 0;
      for (const auto &item : groups)
        index[item] = current++;
      if (scipp::size(groups) != scipp::size(index))
        throw std::runtime_error("Duplicate group labels.");
      return index;
    }};

template <class T>
using group_index_arg = std::tuple<T, std::unordered_map<T, scipp::index>>;

static constexpr auto group_index = overloaded{
    element::arg_list<group_index_arg<int64_t>, group_index_arg<int32_t>,
                      group_index_arg<std::string>>,
    [](const units::Unit &coord, const units::Unit &groups) {
      expect::equals(coord, groups);
      return units::one;
    },
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](const auto &x, const auto &groups) -> scipp::index {
      const auto it = groups.find(x);
      return it == groups.end() ? -1 : it->second;
    }};

static constexpr auto bin_index_to_full_index = overloaded{
    element::arg_list<std::tuple<scipp::span<scipp::index>, scipp::index>>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](auto &size, auto &index) {
      if (index < 0)
        return;
      index += size[index]++;
    }};

} // namespace scipp::core::element
