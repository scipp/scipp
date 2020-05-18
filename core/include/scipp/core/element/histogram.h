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

static constexpr auto histogram = overloaded{
    [](const auto &data, const auto &events, const auto &weights,
       const auto &edges) {
      zero(data);
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

} // namespace scipp::core::element
