// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"

#include "../variable/operations_common.h"
#include "scipp/core/element/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/transform.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {
namespace {
Variable applyMask(const VariableConstView &var, const Variable &masks) {
  return scipp::variable::transform(var, masks,
                                    scipp::core::element::convertMaskedToZero);
}

} // namespace

Variable sum(const VariableConstView &var, const Dim dim,
             const MasksConstView &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    return sum(applyMask(var, mask_union), dim);
  }
  return sum(var, dim);
}

VariableView sum(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks, const VariableView &out) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    return sum(applyMask(var, mask_union), dim, out);
  }
  return sum(var, dim, out);
}

Variable nansum(const VariableConstView &var, const Dim dim,
                const MasksConstView &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    return nansum(applyMask(var, mask_union), dim);
  }
  return nansum(var, dim);
}

VariableView nansum(const VariableConstView &var, const Dim dim,
                    const MasksConstView &masks, const VariableView &out) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    return nansum(applyMask(var, mask_union), dim, out);
  }
  return nansum(var, dim, out);
}

Variable mean(const VariableConstView &var, const Dim dim,
              const MasksConstView &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    return mean_impl(applyMask(var, mask_union), dim, sum(~mask_union, dim));
  }
  return mean(var, dim);
}

Variable nanmean(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks) {
  using variable::isfinite;
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    const auto count = sum(applyMask(isfinite(var), mask_union), dim);
    return nanmean_impl(applyMask(var, mask_union), dim, count);
  }
  return nanmean(var, dim);
}

/// Merges all the masks that have all their dimensions found in the given set
//  of dimensions.
Variable masks_merge_if_contained(const MasksConstView &masks,
                                  const Dimensions &dims) {
  auto mask_union = makeVariable<bool>(Values{false});
  for (const auto &mask : masks) {
    if (dims.contains(mask.second.dims()))
      mask_union = mask_union | mask.second;
  }
  return mask_union;
}

} // namespace scipp::dataset
