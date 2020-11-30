// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/map_view.h"

#include "scipp/core/element/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/transform.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

Variable applyMask(const VariableConstView &var, const Variable &masks) {
  return scipp::variable::transform(var, masks,
                                    scipp::core::element::convertMaskedToZero);
}

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
    const auto masks_sum = sum(mask_union, dim);
    const auto maskedVar = applyMask(var, mask_union);
    return mean_impl(maskedVar, dim, masks_sum);
  }
  return mean(var, dim);
}

VariableView mean(const VariableConstView &var, const Dim dim,
                  const MasksConstView &masks, const VariableView &out) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    const auto masks_sum = sum(mask_union, dim);
    const auto maskedVar = applyMask(var, mask_union);
    return mean_impl(maskedVar, dim, masks_sum, out);
  }
  return mean(var, dim, out);
}

Variable nanmean(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    const auto masks_sum = sum(mask_union, dim);
    const auto maskedVar = applyMask(var, mask_union);
    return nanmean_impl(maskedVar, dim, masks_sum);
  }
  return nanmean(var, dim);
}

VariableView nanmean(const VariableConstView &var, const Dim dim,
                     const MasksConstView &masks, const VariableView &out) {
  if (const auto mask_union = irreducible_mask(masks, dim)) {
    const auto masks_sum = sum(mask_union, dim);
    const auto maskedVar = applyMask(var, mask_union);
    return nanmean_impl(maskedVar, dim, masks_sum, out);
  }
  return nanmean(var, dim, out);
}

Variable nanmean(const VariableConstView &var, const MasksConstView &masks) {
  auto mask_union = masks_merge_if_contained(masks, var.dims());
  auto applied_mask = scipp::variable::transform(
      ~isnan(var), mask_union, scipp::core::element::convertMaskedToZero);
  return nansum(var) / sum(applied_mask);
}

/// Returns the union of all masks with irreducible dimension `dim`.
///
/// Irreducible means that a reduction operation must apply these masks since
/// depend on the reduction dimension. Returns an invalid (empty) variable if
/// there is no irreducible mask.
Variable irreducible_mask(const MasksConstView &masks, const Dim dim) {
  Variable union_;
  for (const auto &mask : masks)
    if (mask.second.dims().contains(dim))
      union_ = union_ ? union_ | mask.second : Variable(mask.second);
  return union_;
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

/// Count element contributions from input var, discounts masked and NaN
/// elements
Variable scale_divisor(const VariableConstView &var,
                       const MasksConstView &masks) {
  auto mask_union = masks_merge_if_contained(masks, var.dims());
  auto applied_mask = transform(~isnan(var), mask_union,
                                scipp::core::element::convertMaskedToZero);
  return sum(applied_mask);
}

} // namespace scipp::dataset
