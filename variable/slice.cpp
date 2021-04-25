// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold, Simon Heybrock
#include <algorithm>

#include "scipp/units/dim.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/slice.h"
#include "scipp/variable/string.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

namespace {

scipp::index get_count(const Variable &coord, const Dim dim,
                       const Variable &value, const bool ascending) {
  return (ascending ? sum(less_equal(coord, value), dim)
                    : sum(greater_equal(coord, value), dim))
      .value<scipp::index>();
}

scipp::index get_index(const Variable &coord, const Dim dim,
                       const Variable &value, const bool ascending,
                       const bool edges) {
  auto i = get_count(coord, dim, value, edges == ascending);
  i = edges ? i - 1 : coord.dims()[dim] - i;
  return std::clamp<scipp::index>(0, i, coord.dims()[dim]);
}

const Variable &get_1d_coord(const Variable &coord) {
  if (coord.dims().ndim() != 1)
    throw except::DimensionError("Multi-dimensional coordinates cannot be used "
                                 "for label-based indexing.");
  return coord;
}

auto get_coord(const Variable &coord, const Dim dim) {
  get_1d_coord(coord);
  const bool ascending = issorted(coord, dim, variable::SortOrder::Ascending);
  const bool descending = issorted(coord, dim, variable::SortOrder::Descending);
  if (!(ascending ^ descending))
    throw std::runtime_error("Coordinate must be monotonically increasing or "
                             "decreasing for label-based indexing.");
  return std::tuple(coord, ascending);
}

} // namespace

std::tuple<Dim, scipp::index> get_slice_params(const Dimensions &dims,
                                               const Variable &coord_,
                                               const Variable &value) {
  core::expect::equals(value.dims(), Dimensions{});
  const auto dim = coord_.dims().inner();
  if (dims[dim] + 1 == coord_.dims()[dim]) {
    const auto &[coord, ascending] = get_coord(coord_, dim);
    return std::tuple{dim, get_count(coord, dim, value, ascending) - 1};
  } else {
    auto eq = equal(get_1d_coord(coord_), value);
    if (sum(eq, dim).template value<scipp::index>() != 1)
      throw except::SliceError("Coord " + to_string(dim) +
                               " does not contain unique point with value " +
                               to_string(value) + '\n');
    auto values = eq.template values<bool>();
    auto it = std::find(values.begin(), values.end(), true);
    return {dim, std::distance(values.begin(), it)};
  }
}

std::tuple<Dim, scipp::index, scipp::index>
get_slice_params(const Dimensions &dims, const Variable &coord_,
                 const Variable &begin, const Variable &end) {
  if (begin)
    core::expect::equals(begin.dims(), Dimensions{});
  if (end)
    core::expect::equals(end.dims(), Dimensions{});
  const auto dim = coord_.dims().inner();
  const auto &[coord, ascending] = get_coord(coord_, dim);
  scipp::index first = 0;
  scipp::index last = dims[dim];
  const auto bin_edges = last + 1 == coord.dims()[dim];
  if (begin)
    first = get_index(coord, dim, begin, ascending, bin_edges);
  if (end)
    last = get_index(coord, dim, end, ascending, bin_edges);
  // Note: Here the bin containing `end` is included
  return {dim, first, std::min(dims[dim], last + (bin_edges ? 1 : 0))};
}

} // namespace scipp::variable
