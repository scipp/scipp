// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"

#include "../variable/operations_common.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {
Variable zero_like(const Variable &var) { return {var, Dimensions{}}; }
} // namespace

Variable sum(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return sum(where(mask_union, zero_like(var), var), dim);
  }
  return sum(var, dim);
}

Variable &sum(const Variable &var, const Dim dim, const Masks &masks,
              Variable &out) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return sum(where(mask_union, zero_like(var), var), dim, out);
  }
  return sum(var, dim, out);
}

Variable nansum(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return nansum(where(mask_union, zero_like(var), var), dim);
  }
  return nansum(var, dim);
}

Variable &nansum(const Variable &var, const Dim dim, const Masks &masks,
                 Variable &out) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return nansum(where(mask_union, zero_like(var), var), dim, out);
  }
  return nansum(var, dim, out);
}

Variable mean(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return mean_impl(where(mask_union, zero_like(var), var), dim,
                     sum(~mask_union, dim));
  }
  return mean(var, dim);
}

Variable nanmean(const Variable &var, const Dim dim, const Masks &masks) {
  using variable::isfinite;
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    const auto count = sum(
        where(mask_union, makeVariable<bool>(Values{false}), ~isnan(var)), dim);
    return nanmean_impl(where(mask_union, zero_like(var), var), dim, count);
  }
  return nanmean(var, dim);
}

/// Merges all the masks that have all their dimensions found in the given set
//  of dimensions.
Variable masks_merge_if_contained(const Masks &masks, const Dimensions &dims) {
  auto mask_union = makeVariable<bool>(Values{false});
  for (const auto &mask : masks) {
    if (dims.includes(mask.second.dims()))
      mask_union = mask_union | mask.second;
  }
  return mask_union;
}

} // namespace scipp::dataset
