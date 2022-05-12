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
#include "scipp/variable/util.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {
// Uses variable::special like but constructs only a scalar
// instead of a full-sized array.
Variable mask_fill(const Variable &prototype, const FillValue fill) {
  return special_like(zero_like(prototype), fill);
}

template <class Op>
Variable reduce_impl(const Variable &var, const Dim dim, const Masks &masks,
                     const FillValue fill, const Op &op) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    return op(where(mask_union, mask_fill(var, fill), var), dim);
  }
  return op(var, dim);
}
} // namespace

Variable sum(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Default,
                     [](auto &&... args) { return sum(args...); });
}

Variable nansum(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Default,
                     [](auto &&... args) { return nansum(args...); });
}

Variable max(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Lowest,
                     [](auto &&... args) { return max(args...); });
}

Variable nanmax(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Lowest,
                     [](auto &&... args) { return nanmax(args...); });
}

Variable min(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Max,
                     [](auto &&... args) { return min(args...); });
}

Variable nanmin(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Max,
                     [](auto &&... args) { return nanmin(args...); });
}

Variable mean(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    const auto count = sum(~mask_union, dim);
    return mean_impl(where(mask_union, zero_like(var), var), dim, count);
  }
  return mean(var, dim);
}

Variable nanmean(const Variable &var, const Dim dim, const Masks &masks) {
  if (const auto mask_union = irreducible_mask(masks, dim);
      mask_union.is_valid()) {
    const auto count = sum(
        where(mask_union, makeVariable<bool>(Values{false}), ~isnan(var)), dim);
    return nanmean_impl(where(mask_union, zero_like(var), var), dim, count);
  }
  return nanmean(var, dim);
}

} // namespace scipp::dataset
