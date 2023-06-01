// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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

namespace {
auto format_coord_mismatch_message(const Dim dim, const Variable &a,
                                   const Variable &b,
                                   const std::string_view opname) {
  std::string message = "Mismatch in coordinate '" + to_string(dim);
  if (!opname.empty())
    message += "' in operation '" + std::string(opname);
  message += "':\n" + format_variable(a) + "\nvs\n" + format_variable(b);
  return message;
}
} // namespace

CoordMismatchError::CoordMismatchError(const Dim dim, const Variable &a,
                                       const Variable &b,
                                       const std::string_view opname)
    : DatasetError{format_coord_mismatch_message(dim, a, b, opname)} {}

} // namespace scipp::except

namespace scipp::expect {
template <>
void contains(const scipp::dataset::Dataset &a, const std::string &b) {
  if (!a.contains(b))
    throw except::NotFoundError("Expected '" + b + "' in " +
                                scipp::dataset::dict_keys_to_string(a) + ".");
}
} // namespace scipp::expect

namespace scipp::dataset::expect {
void coords_are_superset(const Coords &a_coords, const Coords &b_coords,
                         const std::string_view opname) {
  for (const auto &[key, b_coord] : b_coords) {
    if (!a_coords.contains(key)) {
      if (b_coord.is_aligned())
        throw except::CoordMismatchError(
            "Expected coord " + to_string(key) + " in " + to_string(a_coords) +
            " in operation '" + std::string(opname) + '\'');
      else
        continue;
    }
    const auto &a_coord = a_coords[key];
    if (a_coord.is_aligned() && b_coord.is_aligned() && a_coord != b_coord)
      throw except::CoordMismatchError(key, a_coord, b_coord, opname);
  }
}

void coords_are_superset(const DataArray &a, const DataArray &b,
                         const std::string_view opname) {
  coords_are_superset(a.coords(), b.coords(), opname);
}

void matching_coord(const Dim dim, const Variable &a, const Variable &b,
                    const std::string_view opname) {
  if (!equals_nan(a, b))
    throw except::CoordMismatchError(dim, a, b, opname);
}

void is_key(const Variable &key) {
  if (key.dims().ndim() != 1)
    throw except::DimensionError(
        "Coord for binning or grouping must be 1-dimensional");
  if (key.has_variances())
    throw except::VariancesError(
        "Coord for binning or grouping cannot have variances");
}

} // namespace scipp::dataset::expect
