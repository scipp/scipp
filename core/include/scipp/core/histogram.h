// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_HISTOGRAM_H
#define SCIPP_CORE_HISTOGRAM_H

#include <algorithm>

#include "scipp/core/except.h"

namespace scipp::core {

SCIPP_CORE_EXPORT bool is_histogram(const DataConstProxy &a, const Dim dim);

/// Return params for computing bin index for linear edges (constant bin width).
constexpr static auto linear_edge_params = [](const auto &edges) {
  auto len = scipp::size(edges) - 1;
  const auto offset = edges.front();
  static_assert(
      std::is_floating_point_v<decltype(offset)>,
      "linear_bin_edges is not implement to support integer-valued bin-edges");
  const auto nbin = static_cast<decltype(offset)>(len);
  const auto scale = nbin / (edges.back() - edges.front());
  return std::array{offset, nbin, scale};
};

namespace expect::histogram {
template <class T> void sorted_edges(const T &edges) {
  if (!std::is_sorted(edges.begin(), edges.end()))
    throw except::BinEdgeError("Bin edges of histogram must be sorted.");
}
} // namespace expect::histogram

} // namespace scipp::core

#endif // SCIPP_CORE_HISTOGRAM_H
