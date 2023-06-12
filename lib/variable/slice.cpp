// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  if (coord.dims()[dim] == 1)
    // Need this because issorted returns false for length-1 variables.
    return std::tuple(coord, true);
  const bool ascending = allsorted(coord, dim, SortOrder::Ascending);
  const bool descending = allsorted(coord, dim, SortOrder::Descending);
  if (!(ascending ^ descending))
    throw std::runtime_error("Coordinate must be monotonically increasing or "
                             "decreasing for label-based indexing.");
  return std::tuple(coord, ascending);
}

void expect_same_unit(const Variable &coord, const Variable &value,
                      const std::string &name) {
  if (coord.unit() != value.unit()) {
    throw except::UnitError("The unit of the slice " + name + " (" +
                            to_string(value.unit()) +
                            ") does not match the unit of the coordinate (" +
                            to_string(coord.unit()) + ").");
  }
}

void expect_valid_dtype(const Variable &var, const bool is_range,
                        const std::string &name) {
  if (is_range && !is_total_orderable(var.dtype())) {
    throw except::TypeError(
        "The dtype of the slice " + name + " (" + to_string(var.dtype()) +
        ") cannot be used for label-based slicing because it does not"
        " define an order.");
  }
}

void expect_valid_slice_value(const Variable &coord, const Variable &value,
                              const bool is_range,
                              const std::string_view name) {
  if (value.is_valid()) {
    core::expect::equals(Dimensions{}, value.dims());
    expect_same_unit(coord, value, std::string(name));
    expect_valid_dtype(value, is_range, std::string(name));
  }
}
} // namespace

std::tuple<Dim, scipp::index> get_slice_params(const Sizes &dims,
                                               const Variable &coord_,
                                               const Variable &value) {
  expect_valid_slice_value(coord_, value, false, "key");
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
get_slice_params(const Sizes &dims, const Variable &coord_,
                 const Variable &begin, const Variable &end) {
  expect_valid_slice_value(coord_, begin, true, "begin");
  expect_valid_slice_value(coord_, end, true, "end");
  expect_valid_dtype(coord_, true, "coord");
  const auto dim = coord_.dims().inner();
  const auto &[coord, ascending] = get_coord(coord_, dim);
  scipp::index first = 0;
  scipp::index last = dims[dim];
  const auto bin_edges = last + 1 == coord.dims()[dim];
  if (begin.is_valid())
    first = get_index(coord, dim, begin, ascending, bin_edges);
  if (end.is_valid())
    last = get_index(coord, dim, end, ascending, false);
  return {dim, first, std::min(dims[dim], last)};
}

} // namespace scipp::variable
