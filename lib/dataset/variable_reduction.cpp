// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"

#include "../variable/operations_common.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

Variable sum(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return sum(where(mask_union, zero_like(var), var), dim);
  }
  return sum(var, dim);
}

Variable &sum(const Variable &var, const Dim dim, const Masks &masks,
              // cppcheck-suppress constParameter  # intentional out param
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

Variable mean(const Variable &var, const Dim dim, const Masks &masks) {
  if (auto mask_union = irreducible_mask(masks, dim); mask_union.is_valid()) {
    mask_union.setUnit(units::one);
    return mean_impl(where(mask_union, zero_like(var), var), dim,
                     sum(~mask_union, dim));
  }
  return mean(var, dim);
}

Variable nanmean(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    auto count = sum(
        where(mask_union, makeVariable<bool>(Values{false}), ~isnan(var)), dim);
    count.setUnit(units::one);
    return nanmean_impl(where(mask_union, zero_like(var), var), dim, count);
  }
  return nanmean(var, dim);
}

} // namespace scipp::dataset
