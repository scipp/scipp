#include "scipp/dataset/slice.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/dim.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include <iostream>

namespace scipp::dataset {
DataArrayConstView slice(const DataArrayConstView &to_slice, const Dim dim,
                         const VariableConstView begin,
                         const VariableConstView end) {
  using namespace variable;
  auto coords = to_slice.coords();
  if (!coords.contains(dim))
    throw except::DimensionNotFoundError(to_slice.dims(), dim);
  auto [k, coord] = *coords.find(dim);
  if (coord.dims().ndim() != 1) {
    throw except::SizeError(
        "multi-dimensional coordinates not supported in slice");
  }
  const bool ascending = is_sorted(coord, dim, variable::SortOrder::Ascending);
  const bool descending =
      is_sorted(coord, dim, variable::SortOrder::Descending);

  if (!(ascending ^ descending))
    throw std::runtime_error("Coordinate must be monotomically increasing or "
                             "decreasing for value slicing");

  const auto len_data = to_slice.dims()[dim];
  const auto len = coord.dims().volume();
  const auto bin_edges = len == len_data + 1;

  // Point slice
  if ((begin && end) && begin == end) {
    scipp::index idx = -1;
    if (bin_edges) {
      if (ascending)
        idx = sum(less_equal(coord, begin), dim).value<scipp::index>();
      else
        idx = sum(greater_equal(coord, begin), dim).value<scipp::index>();
      if (idx < 0)
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
    scipp::index last = len - 1;
    if (begin) {
      if (bin_edges) {
        // lower bin edge boundary
        if (ascending) {
          first = sum(less_equal(coord, begin), dim).value<scipp::index>() - 1;
        } else {
          first =
              sum(greater_equal(coord, begin), dim).value<scipp::index>() - 1;
        }
      } else {
        // First point >= value for non bin edges
        if (ascending)
          first =
              len - sum(greater_equal(coord, begin), dim).value<scipp::index>();
        else
          first =
              len - sum(less_equal(coord, begin), dim).value<scipp::index>();
      }
      if (first < 0)
        first = 0;
    }
    if (end) {
      if (ascending) {
        last = len - sum(greater_equal(coord, end), dim).value<scipp::index>();
      } else {
        last = len - sum(less_equal(coord, end), dim).value<scipp::index>();
      }
      if (last > len_data)
        last = len - 1;
    }
    return to_slice.slice({dim, first, last});
  }
}
} // namespace scipp::dataset
