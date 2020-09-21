// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold, Simon Heybrock
#include <algorithm>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/slice.h"
#include "scipp/units/dim.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {

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

auto get_1d_coord(const DataArrayConstView &data, const Dim dim) {
  const auto &coord = data.coords()[dim];
  if (coord.dims().ndim() != 1)
    throw except::DimensionError(
        "Multi-dimensional coordinates cannot be used for value-base slicing.");
  return coord;
}

auto get_coord(const DataArrayConstView &data, const Dim dim) {
  const auto &coord = get_1d_coord(data, dim);
  const bool ascending = is_sorted(coord, dim, variable::SortOrder::Ascending);
  const bool descending =
      is_sorted(coord, dim, variable::SortOrder::Descending);
  if (!(ascending ^ descending))
    throw std::runtime_error("Coordinate must be monotonically increasing or "
                             "decreasing for value-based slicing");
  return std::tuple(coord, ascending);
}

template <class T>
T slice(const T &data, const Dim dim, const VariableConstView value) {
  core::expect::equals(value.dims(), Dimensions{});
  if (is_histogram(data, dim)) {
    const auto &[coord, ascending] = get_coord(data, dim);
    return data.slice({dim, get_count(coord, dim, value, ascending) - 1});
  } else {
    auto eq = equal(get_1d_coord(data, dim), value);
    if (sum(eq, dim).template value<scipp::index>() != 1)
      throw except::SliceError("Coord " + to_string(dim) +
                               " does not contain unique point with value " +
                               to_string(value) + '\n');
    auto values = eq.template values<bool>();
    auto it = std::find(values.begin(), values.end(), true);
    return data.slice({dim, std::distance(values.begin(), it)});
  }
}

template <class T>
T slice(const T &data, const Dim dim, const VariableConstView begin,
        const VariableConstView end) {
  if (begin)
    core::expect::equals(begin.dims(), Dimensions{});
  if (end)
    core::expect::equals(end.dims(), Dimensions{});
  const auto &[coord, ascending] = get_coord(data, dim);
  const auto bin_edges = is_histogram(data, dim);
  scipp::index first = 0;
  scipp::index last = data.dims()[dim];
  if (begin)
    first = get_index(coord, dim, begin, ascending, bin_edges);
  if (end)
    last = get_index(coord, dim, end, ascending, bin_edges);
  return data.slice({dim, first, last});
}

} // namespace

DataArrayConstView slice(const DataArrayConstView &data, const Dim dim,
                         const VariableConstView value) {
  return slice<DataArrayConstView>(data, dim, value);
}

DataArrayView slice(const DataArrayView &data, const Dim dim,
                    const VariableConstView value) {
  return slice<DataArrayView>(data, dim, value);
}

DataArrayConstView slice(const DataArrayConstView &data, const Dim dim,
                         const VariableConstView begin,
                         const VariableConstView end) {
  return slice<DataArrayConstView>(data, dim, begin, end);
}

DataArrayView slice(const DataArrayView &data, const Dim dim,
                    const VariableConstView begin,
                    const VariableConstView end) {
  return slice<DataArrayView>(data, dim, begin, end);
}

DataArrayView slice(DataArray &data, const Dim dim,
                    const VariableConstView value) {
  return slice(DataArrayView(data), dim, value);
}

DataArrayView slice(DataArray &data, const Dim dim,
                    const VariableConstView begin,
                    const VariableConstView end) {
  return slice(DataArrayView(data), dim, begin, end);
}
} // namespace scipp::dataset
