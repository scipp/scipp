// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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

scipp::index get_count(const VariableConstView &coord, const Dim dim,
                       const VariableConstView &value, const bool ascending) {
  return (ascending ? sum(less_equal(coord, value), dim)
                    : sum(greater_equal(coord, value), dim))
      .value<scipp::index>();
}

scipp::index get_index(const VariableConstView &coord, const Dim dim,
                       const VariableConstView &value, const bool ascending,
                       const bool edges) {
  auto i = get_count(coord, dim, value, edges ? ascending : !ascending);
  i = edges ? i - 1 : coord.dims()[dim] - i;
  return std::clamp<scipp::index>(0, i, coord.dims()[dim]);
}

auto get_1d_coord(const VariableConstView &coord) {
  if (coord.dims().ndim() != 1)
    throw except::DimensionError(
        "Multi-dimensional coordinates cannot be used for value-based slicing");
  return coord;
}

auto get_coord(VariableConstView coord, const Dim dim) {
  coord = get_1d_coord(coord);
  const bool ascending = is_sorted(coord, dim, variable::SortOrder::Ascending);
  const bool descending =
      is_sorted(coord, dim, variable::SortOrder::Descending);
  if (!(ascending ^ descending))
    throw std::runtime_error("Coordinate must be monotonically increasing or "
                             "decreasing for value-based slicing");
  return std::tuple(coord, ascending);
}

} // namespace

std::tuple<Dim, scipp::index> get_slice_params(const Dimensions &dims,
                                               const VariableConstView &coord_,
                                               const VariableConstView value) {
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
get_slice_params(const Dimensions &dims, const VariableConstView &coord_,
                 const VariableConstView begin, const VariableConstView end) {
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
  return {dim, first, std::min(dims[dim], last + (bin_edges ? 1 : 0))};
}

} // namespace scipp::variable
