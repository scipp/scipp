// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"

#include "scipp/variable/reduction.h"

#include "scipp/dataset/except.h"
#include "scipp/dataset/reduction.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray sum(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return sum(_...); }, dim, a.masks());
}

Dataset sum(const DatasetConstView &d, const Dim dim) {
  // Currently not supporting sum/mean of dataset if one or more items do not
  // depend on the input dimension. The definition is ambiguous (return
  // unchanged, vs. compute sum of broadcast) so it is better to avoid this for
  // now.
  return apply_to_items(
      d, [](auto &&... _) { return sum(_...); }, dim);
}

DataArray mean(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return mean(_...); }, dim, a.masks());
}

Dataset mean(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return mean(_...); }, dim);
}

} // namespace scipp::dataset
