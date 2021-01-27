// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold, Simon Heybrock
#include "scipp/dataset/slice.h"
#include "scipp/variable/slice.h"

namespace scipp::dataset {

namespace {

template <class T>
T slice(const T &data, const Dim dim, const VariableConstView value) {
  const auto [d, i] = get_slice_params(data.dims(), data.coords()[dim], value);
  return data.slice({d, i});
}

template <class T>
T slice(const T &data, const Dim dim, const VariableConstView begin,
        const VariableConstView end) {
  const auto [d, b, e] =
      get_slice_params(data.dims(), data.coords()[dim], begin, end);
  return data.slice({d, b, e});
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
DatasetConstView slice(const DatasetConstView &ds, const Dim dim,
                       const VariableConstView value) {
  return slice<DatasetConstView>(ds, dim, value);
}
DatasetView slice(const DatasetView &ds, const Dim dim,
                  const VariableConstView value) {
  return slice<DatasetView>(ds, dim, value);
}
DatasetView slice(Dataset &ds, const Dim dim, const VariableConstView value) {
  return slice(DatasetView(ds), dim, value);
}

DatasetConstView slice(const DatasetConstView &ds, const Dim dim,
                       const VariableConstView begin,
                       const VariableConstView end) {
  return slice<DatasetConstView>(ds, dim, begin, end);
}

DatasetView slice(const DatasetView &ds, const Dim dim,
                  const VariableConstView begin, const VariableConstView end) {
  return slice<DatasetView>(ds, dim, begin, end);
}

DatasetView slice(Dataset &ds, const Dim dim, const VariableConstView begin,
                  const VariableConstView end) {
  return slice(DatasetView(ds), dim, begin, end);
}

} // namespace scipp::dataset
