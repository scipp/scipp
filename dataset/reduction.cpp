// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/reduction.h"
#include "dataset_operations_common.h"
#include "scipp/core/element/util.h"
#include "scipp/dataset/math.h"
#include "scipp/dataset/reduction.h"
#include "scipp/dataset/special_values.h"

using scipp::common::reduce_all_dims;

namespace scipp::dataset {

DataArray sum(const DataArrayConstView &a) {
  return reduce_all_dims(a, [](auto &&... _) { return sum(_...); });
}
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

Dataset sum(const DatasetConstView &d) {
  return apply_to_items(d, [](auto &&... _) { return sum(_...); });
}

DataArray nansum(const DataArrayConstView &a) {
  return reduce_all_dims(a, [](auto &&... _) { return nansum(_...); });
}

DataArray nansum(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return nansum(_...); }, dim, a.masks());
}

Dataset nansum(const DatasetConstView &d, const Dim dim) {
  // Currently not supporting sum/mean of dataset if one or more items do not
  // depend on the input dimension. The definition is ambiguous (return
  // unchanged, vs. compute sum of broadcast) so it is better to avoid this for
  // now.
  return apply_to_items(
      d, [](auto &&... _) { return nansum(_...); }, dim);
}

Dataset nansum(const DatasetConstView &d) {
  return apply_to_items(d, [](auto &&... _) { return nansum(_...); });
}

DataArray mean(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return mean(_...); }, dim, a.masks());
}

DataArray mean(const DataArrayConstView &a) {
  auto _sum = sum(a);
  if (isInt(a.data().dtype()))
    return sum(a) * reciprocal(astype(sum(isfinite(a)), dtype<double>));
  else
    return sum(a) * reciprocal(astype(sum(isfinite(a)), a.dtype()));
}

Dataset mean(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return mean(_...); }, dim);
}

Dataset mean(const DatasetConstView &d) {
  return apply_to_items(d, [](auto &&... _) { return mean(_...); });
}

DataArray nanmean(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return nanmean(_...); }, dim, a.masks());
}

DataArray nanmean(const DataArrayConstView &a) {
  if (isInt(a.data().dtype()))
    return mean(a);
  return nansum(a) * reciprocal(astype(sum(isfinite(a)), a.dtype()));
}

Dataset nanmean(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return nanmean(_...); }, dim);
}

Dataset nanmean(const DatasetConstView &d) {
  return apply_to_items(d, [](auto &&... _) { return nanmean(_...); });
}

} // namespace scipp::dataset
