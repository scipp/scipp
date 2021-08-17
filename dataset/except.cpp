// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/except.h"
#include "scipp/dataset/dataset.h"

namespace scipp::except {

DataArrayError::DataArrayError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const dataset::DataArray &expected,
                          const dataset::DataArray &actual,
                          const std::string &optional_message) {
  throw DataArrayError("Expected DataArray " + to_string(expected) + ", got " +
                       to_string(actual) + '.' + optional_message);
}

DatasetError::DatasetError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const dataset::Dataset &expected,
                          const dataset::Dataset &actual,
                          const std::string &optional_message) {
  throw DatasetError("Expected Dataset " + to_string(expected) + ", got " +
                     to_string(actual) + '.' + optional_message);
}

CoordMismatchError::CoordMismatchError(const Dim dim, const Variable &expected,
                                       const Variable &actual)
    : DatasetError{"Mismatch in coordinate '" + to_string(dim) +
                   "', expected\n" + format_variable(expected) + ", got\n" +
                   format_variable(actual)} {}

CoordMismatchError::CoordMismatchError(const std::string &message)
    : DatasetError{message} {}

} // namespace scipp::except

namespace scipp::dataset::expect {
void coordsAreSuperset(const Coords &a_coords, const Coords &b_coords) {
  for (const auto &b_coord : b_coords) {
    if (a_coords[b_coord.first] != b_coord.second)
      throw except::CoordMismatchError(b_coord.first, a_coords[b_coord.first],
                                       b_coord.second);
  }
}

void coordsAreSuperset(const DataArray &a, const DataArray &b) {
  coordsAreSuperset(a.coords(), b.coords());
}

void matchingCoord(const Dim dim, const Variable &a, const Variable &b) {
  if (a != b)
    throw except::CoordMismatchError(dim, a, b);
}

void isKey(const Variable &key) {
  if (key.dims().ndim() != 1)
    throw except::DimensionError(
        "Coord for binning or grouping must be 1-dimensional");
  if (key.hasVariances())
    throw except::VariancesError(
        "Coord for binning or grouping cannot have variances");
}

} // namespace scipp::dataset::expect
