// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "../variable/operations_common.h"
#include "scipp/dataset/sized_dict.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/util.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {
template <class Op>
Variable reduce_impl(const Variable &var, const Dim dim, const Masks &masks,
                     const FillValue fill, const Op &op) {
  if (auto mask_union = irreducible_mask(masks, dim); mask_union.is_valid()) {
    mask_union = transpose(
        mask_union, intersection(var.dims(), mask_union.dims()).labels());
    return op(
        where(mask_union, dense_special_like(var, Dimensions{}, fill), var),
        dim);
  }
  return op(var, dim);
}
} // namespace

Variable sum(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Default,
                     [](auto &&...args) { return sum(args...); });
}

Variable nansum(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Default,
                     [](auto &&...args) { return nansum(args...); });
}

Variable max(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Lowest,
                     [](auto &&...args) { return max(args...); });
}

Variable nanmax(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Lowest,
                     [](auto &&...args) { return nanmax(args...); });
}

Variable min(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Max,
                     [](auto &&...args) { return min(args...); });
}

Variable nanmin(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::Max,
                     [](auto &&...args) { return nanmin(args...); });
}

Variable all(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::True,
                     [](auto &&...args) { return all(args...); });
}

Variable any(const Variable &var, const Dim dim, const Masks &masks) {
  return reduce_impl(var, dim, masks, FillValue::False,
                     [](auto &&...args) { return any(args...); });
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
