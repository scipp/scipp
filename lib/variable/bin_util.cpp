// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bin_util.h"

namespace scipp::variable {

namespace {
void check_edges(const Variable &edges, const Dim dim) {
  if (edges.dims()[dim] < 2)
    throw except::BinEdgeError("Invalid bin edges: less than 2 values");
}
} // namespace

/// Return view onto left bin edges, i.e., all but the last edge
Variable left_edge(const Variable &edges) {
  const auto dim = edges.dims().inner();
  check_edges(edges, dim);
  return edges.slice({dim, 0, edges.dims()[dim] - 1});
}

/// Return view onto right bin edges, i.e., all but the first edge
Variable right_edge(const Variable &edges) {
  const auto dim = edges.dims().inner();
  check_edges(edges, dim);
  return edges.slice({dim, 1, edges.dims()[dim]});
}

} // namespace scipp::variable
