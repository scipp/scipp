// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"

#include "scipp/variable/reduction.h"

#include "scipp/dataset/except.h"
#include "scipp/dataset/reduction.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray flatten(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a,
      overloaded{no_realigned_support,
                 [](const auto &x, const Dim dim_, const auto &mask_) {
                   if (!contains_events(x) && min(x, dim_) != max(x, dim_))
                     throw except::EventDataError(
                         "flatten with non-constant scalar weights not "
                         "possible yet.");
                   return contains_events(x) ? flatten(x, dim_, mask_)
                                             : copy(x.slice({dim_, 0}));
                 }},
      dim, a.masks());
}

Dataset flatten(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return flatten(_...); }, dim);
}

namespace {
UnalignedData sum(Dimensions dims, const DataArrayConstView &unaligned,
                  const Dim dim, const MasksConstView &masks) {
  static_cast<void>(masks); // relevant masks are part of unaligned as well
  dims.erase(dim);
  return {dims, flatten(unaligned, dim)};
}
} // namespace

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
      a,
      overloaded{no_realigned_support, [](auto &&... _) { return mean(_...); }},
      dim, a.masks());
}

Dataset mean(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return mean(_...); }, dim);
}

} // namespace scipp::dataset
