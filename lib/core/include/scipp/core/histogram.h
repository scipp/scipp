// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

} // namespace scipp::core
