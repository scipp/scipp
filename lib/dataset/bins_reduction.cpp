// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/dataset/bins_reduction.h"

#include "scipp/core/dtype.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "../variable/operations_common.h"
#include "scipp/dataset/data_array.h"
#include "scipp/dataset/dataset.h"

namespace scipp::variable {

namespace {
Variable applyMask(const DataArray &buffer, const Variable &indices,
                   const Dim dim, const Variable &mask) {
  return make_bins(
      indices, dim,
      where(mask, Variable(buffer.data(), Dimensions{}), buffer.data()));
}

Variable dense_zeros_like(const Variable &prototype, const DType type) {
  const auto unit = variable::variableFactory().elem_unit(prototype);
  if (variable::variableFactory().has_variances(prototype))
    return {type, prototype.dims(), unit, Values{}, Variances{}};
  else
    return {type, prototype.dims(), unit, Values{}};
}

Variable reduce_bins(const Variable &data,
                     void (&op)(Variable &, const Variable &),
                     const DType type) {
  auto reduced = dense_zeros_like(data, type);
  if (data.dtype() == dtype<bucket<DataArray>>) {
    const auto &&[indices, dim, buffer] = data.constituents<DataArray>();
    if (const auto mask_union = irreducible_mask(buffer.masks(), dim);
        mask_union.is_valid()) {
      op(reduced, applyMask(buffer, indices, dim, mask_union));
    } else {
      op(reduced, data);
    }
  } else {
    op(reduced, data);
  }
  return reduced;
}
} // namespace

Variable bins_sum(const Variable &data) {
  auto type = variable::variableFactory().elem_dtype(data);
  type = type == dtype<bool> ? dtype<int64_t> : type;
  return reduce_bins(data, variable::sum_into, type);
  //  return reduce_bins(data, [](Variable &accum, const Variable &d)
  //  {variable::sum_into(accum, d);}, type);
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
