#include "scipp/dataset/slice.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/dim.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

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

  const auto bins = coord.dims().volume();

  const auto bin_edges = bins == to_slice.dims()[dim] + 1;

  scipp::index first = 0;
  scipp::index last = bins - 1;
  // TODO Point slice
  if ((begin && end) && begin == end) {
    throw std::runtime_error("Not implemented yet");
  }
  // Range slice
  else {
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
          first = bins -
                  sum(greater_equal(coord, begin), dim).value<scipp::index>();
        else
          first =
              bins - sum(less_equal(coord, begin), dim).value<scipp::index>();
      }
    }
    if (end) {
      if (ascending) {
        last = bins - sum(greater_equal(coord, end), dim).value<scipp::index>();
      } else {
        last = bins - sum(less_equal(coord, end), dim).value<scipp::index>();
      }
    }

    if (first < 0)
      first = 0;
    if (last > bins)
      last = bins - 1;
    return to_slice.slice(Slice(dim, first, last));
  }
}
} // namespace scipp::dataset
