// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/rebin.h"
#include "scipp/variable/rebin.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray rebin(const DataArray &a, const Dim dim, const Variable &coord) {
  auto rebinned = apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return rebin(_...); }, dim, a.coords()[dim], coord);
  for (auto &&[name, mask] : a.masks()) {
    if (mask.dims().contains(dim))
      rebinned.masks().set(name, rebin(mask, dim, a.coords()[dim], coord));
  }
  rebinned.coords().set(dim, coord);
  return rebinned;
}

Dataset rebin(const Dataset &d, const Dim dim, const Variable &coord) {
  return apply_to_items(
      d, [](auto &&... _) { return rebin(_...); }, dim, coord);
}

} // namespace scipp::dataset
