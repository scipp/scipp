// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/dataset/bins_reduction.h"

#include "scipp/variable/bins.h"
#include "scipp/variable/reduction.h"

#include "../variable/operations_common.h"
#include "scipp/dataset/data_array.h"
#include "scipp/dataset/dataset.h"

namespace scipp::variable {

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
