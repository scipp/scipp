// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/reduction.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/core/element/comparison.h"
#include "scipp/core/element/logical.h"
#include "scipp/variable/accumulate.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {

Variable make_accumulant(const Variable &var, const Dim dim,
                         const FillValue &init) {
  if (variableFactory().has_masks(var))
    throw except::NotImplementedError(
        "Reduction operations for binned data with "
        "event masks not supported yet.");
  auto dims = var.dims();
  dims.erase(dim);
  auto prototype = empty(dims, variableFactory().elem_unit(var),
                         variableFactory().elem_dtype(var),
                         variableFactory().has_variances(var));
  return special_like(prototype, init);
}

/// Note that masking is not supported here since it would make creation
/// of a sensible starting value difficult. So masks must be applied
/// before calling reduce_idempotent.
Variable apply_reduction(const Variable &var, const Dim dim,
                         void (&op)(Variable &, const Variable &),
                         const FillValue &init) {
  auto out = make_accumulant(var, dim, init);
  op(out, var);
  return out;
}
} // namespace

Variable sum(const Variable &var, const Dim dim) {
  // Bool DType is a bit special in that it cannot contain its sum.
  // Instead, the sum is stored in an int64_t Variable
  return apply_reduction(var, dim, sum_into, FillValue::ZeroNotBool);
}

Variable nansum(const Variable &var, const Dim dim) {
  // Bool DType is a bit special in that it cannot contain its sum.
  // Instead, the sum is stored in an int64_t Variable
  return apply_reduction(var, dim, nansum_into, FillValue::ZeroNotBool);
}

Variable any(const Variable &var, const Dim dim) {
  return apply_reduction(var, dim, any_into, FillValue::False);
}

Variable all(const Variable &var, const Dim dim) {
  return apply_reduction(var, dim, all_into, FillValue::True);
}

/// Return the maximum along given dimension.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable max(const Variable &var, const Dim dim) {
  return apply_reduction(var, dim, max_into, FillValue::Lowest);
}

/// Return the maximum along given dimension ignoring NaN values.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable nanmax(const Variable &var, const Dim dim) {
  return apply_reduction(var, dim, nanmax_into, FillValue::Lowest);
}

/// Return the minimum along given dimension.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable min(const Variable &var, const Dim dim) {
  return apply_reduction(var, dim, min_into, FillValue::Max);
}

/// Return the minimum along given dimension ignorning NaN values.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable nanmin(const Variable &var, const Dim dim) {
  return apply_reduction(var, dim, nanmin_into, FillValue::Max);
}

Variable mean_impl(const Variable &var, const Dim dim, const Variable &count) {
  return normalize_impl(sum(var, dim), count);
}

Variable nanmean_impl(const Variable &var, const Dim dim,
                      const Variable &count) {
  return normalize_impl(nansum(var, dim), count);
}

namespace {
template <class... Dim> Variable count(const Variable &var, Dim &&... dim) {
  if (!is_bins(var)) {
    if constexpr (sizeof...(dim) == 0)
      return var.dims().volume() * units::none;
    else
      return ((var.dims()[dim] * units::none) * ...);
  }
  const auto [begin, end] = unzip(var.bin_indices());
  return sum(end - begin, dim...);
}
} // namespace

Variable mean(const Variable &var, const Dim dim) {
  return mean_impl(var, dim, count(var, dim));
}

Variable nanmean(const Variable &var, const Dim dim) {
  return nanmean_impl(var, dim, sum(isfinite(var), dim));
}

/// Return the sum along all dimensions.
Variable sum(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return sum(_...); });
}

/// Return the sum along all dimensions, nans treated as zero.
Variable nansum(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nansum(_...); });
}

/// Return the maximum along all dimensions.
Variable max(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return max(_...); });
}

/// Return the maximum along all dimensions ignorning NaN values.
Variable nanmax(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nanmax(_...); });
}

/// Return the minimum along all dimensions.
Variable min(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return min(_...); });
}

/// Return the minimum along all dimensions ignoring NaN values.
Variable nanmin(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nanmin(_...); });
}

/// Return the logical AND along all dimensions.
Variable all(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return all(_...); });
}

/// Return the logical OR along all dimensions.
Variable any(const Variable &var) {
  return reduce_all_dims(var, [](auto &&... _) { return any(_...); });
}

/// Return the mean along all dimensions.
Variable mean(const Variable &var) {
  return normalize_impl(sum(var), count(var));
}

/// Return the mean along all dimensions. Ignoring NaN values.
Variable nanmean(const Variable &var) {
  return normalize_impl(nansum(var), sum(isfinite(var)));
}

void sum_into(Variable &accum, const Variable &var) {
  if (accum.dtype() == dtype<float>) {
    auto x = astype(accum, dtype<double>);
    sum_into(x, var);
    copy(astype(x, dtype<float>), accum);
  } else {
    accumulate_in_place(accum, var, element::add_equals, "sum");
  }
}

void nansum_into(Variable &summed, const Variable &var) {
  if (summed.dtype() == dtype<float>) {
    auto accum = astype(summed, dtype<double>);
    nansum_into(accum, var);
    copy(astype(accum, dtype<float>), summed);
  } else {
    accumulate_in_place(summed, var, element::nan_add_equals, "nansum");
  }
}

void all_into(Variable &accum, const Variable &var) {
  accumulate_in_place(accum, var, core::element::logical_and_equals, "all");
}

void any_into(Variable &accum, const Variable &var) {
  accumulate_in_place(accum, var, core::element::logical_or_equals, "any");
}

void max_into(Variable &accum, const Variable &var) {
  accumulate_in_place(accum, var, core::element::max_equals, "max");
}

void nanmax_into(Variable &accum, const Variable &var) {
  accumulate_in_place(accum, var, core::element::nanmax_equals, "max");
}

void min_into(Variable &accum, const Variable &var) {
  accumulate_in_place(accum, var, core::element::min_equals, "min");
}

void nanmin_into(Variable &accum, const Variable &var) {
  accumulate_in_place(accum, var, core::element::nanmin_equals, "min");
}

} // namespace scipp::variable
