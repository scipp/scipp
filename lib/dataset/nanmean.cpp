// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/nanmean.h"

#include "scipp/variable/bins.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/astype.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/math.h" // needed by operations_common.h
#include "scipp/dataset/nansum.h"
#include "scipp/dataset/special_values.h"
#include "scipp/dataset/sum.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray nanmean(const DataArray &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return nanmean(_...); }, dim, a.masks());
}

DataArray nanmean(const DataArray &a) {
  return variable::normalize_impl(nansum(a), sum(isfinite(a)));
}

Dataset nanmean(const Dataset &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return nanmean(_...); }, dim);
}

Dataset nanmean(const Dataset &d) {
  return apply_to_items(d, [](auto &&... _) { return nanmean(_...); });
}
} // namespace scipp::dataset

namespace scipp::variable {
namespace {
Variable bin_sizes_without_mask_and_nan(const Variable &data) {
  const auto finite = isfinite(dataset::bins_view<DataArray>(data).data());
  if (const auto mask_union =
          variable::variableFactory().irreducible_event_mask(data);
      mask_union.is_valid()) {
    const auto binned_mask = variable::make_bins_no_validate(
        data.bin_indices(), variable::variableFactory().elem_dim(data),
        ~mask_union);
    return bins_sum(finite & binned_mask);
  }
  return bins_sum(finite);
}
} // namespace

/// Return the mean of all events per bin. Ignoring NaN values.
Variable bins_nanmean(const Variable &data) {
  return normalize_impl(bins_nansum(data),
                        bin_sizes_without_mask_and_nan(data));
}

} // namespace scipp::variable
