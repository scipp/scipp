// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold, Simon Heybrock
#include <sstream>

#include "scipp/dataset/slice.h"
#include "scipp/variable/slice.h"

namespace scipp::dataset {

namespace {

template <class T> const auto &get_coord(const T &data, const Dim dim) {
  const auto &coords = data.coords();
  if (coords.contains(dim))
    return coords[dim];

  std::ostringstream oss;
  oss << "Invalid slice dimension: '" << dim
      << "': no coordinate for that dimension. Coordinates are (";
  for (auto it = coords.keys_begin(); it != coords.keys_end(); ++it) {
    oss << to_string(*it) << ", ";
  }
  oss << ")";
  throw except::DimensionError(oss.str());
}

template <class T>
T slice(const T &data, const Dim dim, const Variable &value) {
  return data.slice(
      std::make_from_tuple<Slice>(get_slice_params(data, dim, value)));
}

template <class T>
T slice(const T &data, const Dim dim, const Variable &begin,
        const Variable &end) {
  return data.slice(
      std::make_from_tuple<Slice>(get_slice_params(data, dim, begin, end)));
}

} // namespace

std::tuple<Dim, scipp::index>
get_slice_params(const DataArray &data, const Dim dim, const Variable &value) {
  return get_slice_params(data.dims(), get_coord(data, dim), value);
}

std::tuple<Dim, scipp::index, scipp::index>
get_slice_params(const DataArray &data, const Dim dim, const Variable &begin,
                 const Variable &end) {
  return get_slice_params(data.dims(), get_coord(data, dim), begin, end);
}

std::tuple<Dim, scipp::index>
get_slice_params(const Dataset &data, const Dim dim, const Variable &value) {
  return get_slice_params(data.dims(), get_coord(data, dim), value);
}

std::tuple<Dim, scipp::index, scipp::index>
get_slice_params(const Dataset &data, const Dim dim, const Variable &begin,
                 const Variable &end) {
  return get_slice_params(data.dims(), get_coord(data, dim), begin, end);
}

DataArray slice(const DataArray &data, const Dim dim, const Variable &value) {
  return slice<DataArray>(data, dim, value);
}

DataArray slice(const DataArray &data, const Dim dim, const Variable &begin,
                const Variable &end) {
  return slice<DataArray>(data, dim, begin, end);
}

Dataset slice(const Dataset &ds, const Dim dim, const Variable &value) {
  return slice<Dataset>(ds, dim, value);
}

Dataset slice(const Dataset &ds, const Dim dim, const Variable &begin,
              const Variable &end) {
  return slice<Dataset>(ds, dim, begin, end);
}

} // namespace scipp::dataset
