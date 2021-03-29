// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold, Simon Heybrock
#include "scipp/dataset/slice.h"
#include "scipp/variable/slice.h"

namespace scipp::dataset {

namespace {

template <class T>
T slice(const T &data, const Dim dim, const Variable &value) {
  const auto [d, i] = get_slice_params(data.dims(), data.coords()[dim], value);
  return data.slice({d, i});
}

template <class T>
T slice(const T &data, const Dim dim, const Variable &begin,
        const Variable &end) {
  const auto [d, b, e] =
      get_slice_params(data.dims(), data.coords()[dim], begin, end);
  return data.slice({d, b, e});
}

} // namespace

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
