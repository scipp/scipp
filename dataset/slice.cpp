// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
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
scipp::index get_index_impl(const VariableConstView &coord, const Dim dim,
                            const VariableConstView &value,
                            const bool ascending) {
  return (ascending ? sum(less_equal(coord, value), dim)
                    : sum(greater_equal(coord, value), dim))
      .value<scipp::index>();
}
scipp::index get_index(const VariableConstView &coord, const Dim dim,
                       const VariableConstView &value, const bool ascending,
                       const bool edges) {
  auto i = get_index_impl(coord, dim, value, edges ? ascending : !ascending);
  i = edges ? i - 1 : coord.dims()[dim] - i;
  return std::clamp<scipp::index>(0, i, coord.dims()[dim]);
}
} // namespace

DataArrayConstView slice(const DataArrayConstView &to_slice, const Dim dim,
                         const VariableConstView begin,
                         const VariableConstView end) {
  using namespace variable;
  if (begin)
    core::expect::equals(begin.dims(), Dimensions{});
  if (end)
    core::expect::equals(end.dims(), Dimensions{});
  const auto &coord = to_slice.coords()[dim];
  if (coord.dims().ndim() != 1) {
    throw except::DimensionError(
        "Multi-dimensional coordinates cannot be used for value-base slicing.");
  }
  const bool ascending = is_sorted(coord, dim, variable::SortOrder::Ascending);
  const bool descending =
      is_sorted(coord, dim, variable::SortOrder::Descending);

  if (!(ascending ^ descending))
    throw std::runtime_error("Coordinate must be monotomically increasing or "
                             "decreasing for value slicing");

  const auto len = to_slice.dims()[dim];
  const auto bin_edges = is_histogram(to_slice, dim);

  // Point slice
  if ((begin && end) && begin == end) {
    scipp::index idx = -1;
    if (bin_edges) {
      if (ascending)
        idx = sum(less_equal(coord, begin), dim).value<scipp::index>() - 1;
      else
        idx = sum(greater_equal(coord, begin), dim).value<scipp::index>() - 1;
      if (idx < 0 || idx >= len)
        throw except::NotFoundError(
            to_string(begin) +
            " point slice does not fall within any bin edges along " +
            to_string(dim));
    } else {
      auto eq = equal(coord, begin);
      auto values_view = eq.values<bool>();
      auto it = std::find(values_view.begin(), values_view.end(), true);
      if (it == values_view.end())
        throw except::NotFoundError(to_string(begin) +
                                    " point slice does not exactly match any "
                                    "point coordinate value along " +
                                    to_string(dim));
      idx = std::distance(values_view.begin(), it);
    }
    return to_slice.slice({dim, idx});
  }
  // Range slice
  else {
    scipp::index first = 0;
    scipp::index last = len;
    if (begin)
      first = get_index(coord, dim, begin, ascending, bin_edges);
    if (end)
      last = get_index(coord, dim, end, ascending, bin_edges);
    return to_slice.slice({dim, first, last});
  }
}
} // namespace scipp::dataset
