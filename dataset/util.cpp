// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#include "scipp/dataset/util.h"

namespace scipp {
SCIPP_DATASET_EXPORT scipp::index size_of(const VariableConstView &view) {
  auto value_size = view.underlying().data().dtype_size();
  auto variance_scale = view.hasVariances() ? 2 : 1;
  return view.dims().volume() * value_size * variance_scale;
}

SCIPP_DATASET_EXPORT scipp::index size_of(const Variable &view) {
  auto value_size = view.data().dtype_size();
  auto variance_scale = view.hasVariances() ? 2 : 1;
  return view.dims().volume() * value_size * variance_scale;
}

/// Return the size in memory of a DataArray object. The aligned coord is
/// optional becuase for a DataArray owned by a dataset aligned coords are
/// assumed to be owned by the dataset as they can apply to multiple arrays.
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArrayConstView &dataarray,
                                          bool include_aligned_coords) {
  scipp::index size = 0;
  size += size_of(dataarray.data());
  for (const auto &coord : dataarray.unaligned_coords()) {
    size += size_of(coord.second);
  }
  for (const auto &mask : dataarray.masks()) {
    size += size_of(mask.second);
  }
  if (include_aligned_coords) {
    for (const auto &coord : dataarray.aligned_coords()) {
      size += size_of(coord.second);
    }
  }
  return size;
}

SCIPP_DATASET_EXPORT scipp::index size_of(const DatasetConstView &dataset) {
  scipp::index size = 0;
  for (const auto &data : dataset) {
    size += size_of(data);
  }
  for (const auto &coord : dataset.coords()) {
    size += size_of(coord.second);
  }
  return size;
}
} // namespace scipp