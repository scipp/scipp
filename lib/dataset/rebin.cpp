// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/rebin.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/rebin.h"
#include "scipp/variable/util.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

Variable rebin(const Variable &var, const Dim dim, const Variable &oldCoord,
               const Variable &newCoord, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return rebin(where(mask_union, zero_like(var), var), dim, oldCoord,
                 newCoord);
  }
  return rebin(var, dim, oldCoord, newCoord);
}

DataArray rebin(const DataArray &a, const Dim dim, const Variable &coord) {
  auto rebinned = apply_to_data_and_drop_dim(
      a, [](auto &&..._) { return rebin(_...); }, dim, a.coords()[dim], coord,
      a.masks());
  rebinned.coords().set(dim, coord);
  return rebinned;
}

Dataset rebin(const Dataset &d, const Dim dim, const Variable &coord) {
  return apply_to_items(d, [](auto &&..._) { return rebin(_...); }, dim, coord);
}

} // namespace scipp::dataset
