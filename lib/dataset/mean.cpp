// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/mean.h"
#include "scipp/dataset/astype.h"
#include "scipp/dataset/math.h" // needed by operations_common.h
#include "scipp/dataset/special_values.h"
#include "scipp/dataset/sum.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray mean(const DataArray &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&..._) { return mean(_...); }, dim, a.masks());
}

DataArray mean(const DataArray &a) {
  return variable::normalize_impl(sum(a), sum(isfinite(a)));
}

Dataset mean(const Dataset &d, const Dim dim) {
  return apply_to_items(d, [](auto &&..._) { return mean(_...); }, dim);
}

Dataset mean(const Dataset &d) {
  return apply_to_items(d, [](auto &&..._) { return mean(_...); });
}

} // namespace scipp::dataset
