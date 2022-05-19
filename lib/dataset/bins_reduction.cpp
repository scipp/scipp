// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/dataset/bins_reduction.h"

#include "scipp/variable/bins.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/reduction.h"

#include "../variable/operations_common.h"
#include "scipp/dataset/data_array.h"
#include "scipp/dataset/dataset.h"

namespace scipp::variable {

namespace {
Variable reduce_bins(const Variable &data,
                     void (&op)(Variable &, const Variable &),
                     const FillValue init) {
  auto reduced = make_reduction_accumulant(data, data.dims(), init);
  reduce_into(reduced, data, op);
  return reduced;
}
} // namespace

Variable bins_sum(const Variable &data) {
  return reduce_bins(data, variable::sum_into, FillValue::ZeroNotBool);
}

Variable bins_max(const Variable &data) {
  return reduce_bins(data, variable::max_into, FillValue::Lowest);
}

Variable bins_nanmax(const Variable &data) {
  return reduce_bins(data, variable::nanmax_into, FillValue::Lowest);
}

Variable bins_min(const Variable &data) {
  return reduce_bins(data, variable::min_into, FillValue::Max);
}

Variable bins_nanmin(const Variable &data) {
  return reduce_bins(data, variable::nanmin_into, FillValue::Max);
}

Variable bins_all(const Variable &data) {
  return reduce_bins(data, variable::all_into, FillValue::True);
}

Variable bins_any(const Variable &data) {
  return reduce_bins(data, variable::any_into, FillValue::False);
}

Variable bins_mean(const Variable &data) {
  if (data.dtype() == dtype<bucket<DataArray>>) {
    const auto &&[indices, dim, buffer] = data.constituents<DataArray>();
    if (const auto mask_union = irreducible_mask(buffer.masks(), dim);
        mask_union.is_valid()) {
      // Trick to get the sizes of bins if masks are present - bin the masks
      // using the same dimension & indices as the data, and then sum the
      // inverse of the mask to get the number of unmasked entries.
      return normalize_impl(bins_sum(data), bins_sum(make_bins_no_validate(
                                                indices, dim, ~mask_union)));
    }
  }
  return normalize_impl(bins_sum(data), bin_sizes(data));
}

} // namespace scipp::variable
