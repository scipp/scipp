// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bin_util.h"

namespace scipp::variable {

namespace {
void check_edges(const VariableConstView &edges, const Dim dim) {
  if (edges.dims()[dim] < 2)
    throw except::BinEdgeError("Invalid bin edges: less than 2 values");
}
} // namespace

/// Return view onto left bin edges, i.e., all but the last edge
VariableConstView left_edge(const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  check_edges(edges, dim);
  return edges.slice({dim, 0, edges.dims()[dim] - 1});
}

/// Return view onto right bin edges, i.e., all but the first edge
VariableConstView right_edge(const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  check_edges(edges, dim);
  return edges.slice({dim, 1, edges.dims()[dim]});
}

} // namespace scipp::variable
