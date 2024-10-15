// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <tuple>

#include "scipp/core/except.h"

namespace scipp::core {

/// Return params for computing bin index for linear edges (constant bin width).
constexpr static auto linear_edge_params = [](const auto &edges) {
  auto len = scipp::size(edges) - 1;
  const auto offset = edges.front();
  const auto nbin = len;
  const auto scale = static_cast<double>(nbin) / (edges.back() - edges.front());
  return std::tuple{offset, nbin, scale};
};

namespace expect::histogram {
template <class T> void sorted_edges(const T &edges) {
  if (!std::is_sorted(edges.begin(), edges.end()))
    throw except::BinEdgeError("Bin edges of histogram must be sorted.");
}
} // namespace expect::histogram

template <class Index, class T, class Edges, class Params>
Index get_bin(const T &x, const Edges &edges, const Params &params) {
  // Explicitly check for x outside edges here as otherwise we may run into an
  // integer overflow when converting the "bin" computation result to `Index`.
  if (x < edges.front() || x >= edges.back())
    return -1;
  // If x is NaN we also return early as it cannot be in any bin.
  if constexpr (!std::is_same_v<T, time_point>)
    if (std::isnan(x))
      return -1;
  const auto [offset, nbin, scale] = params;
  Index bin = (x - offset) * scale;
  bin = std::clamp(bin, Index(0), Index(nbin - 1));
  if (x < edges[bin]) {
    return bin - 1;
  } else if (x >= edges[bin + 1]) {
    return bin + 1;
  } else {
    return bin;
  }
}

} // namespace scipp::core
