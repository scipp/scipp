// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/nanmean.h"

#include "scipp/dataset/astype.h"
#include "scipp/dataset/math.h" // needed by operations_common.h
#include "scipp/dataset/nansum.h"
#include "scipp/dataset/special_values.h"
#include "scipp/dataset/sum.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray nanmean(const DataArray &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&..._) { return nanmean(_...); }, dim, a.masks());
}

DataArray nanmean(const DataArray &a) {
  return variable::normalize_impl(nansum(a), sum(isfinite(a)));
}

Dataset nanmean(const Dataset &d, const Dim dim) {
  return apply_to_items(d, [](auto &&..._) { return nanmean(_...); }, dim);
}

Dataset nanmean(const Dataset &d) {
  return apply_to_items(d, [](auto &&..._) { return nanmean(_...); });
}
} // namespace scipp::dataset
