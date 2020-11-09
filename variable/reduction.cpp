// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/reduction.h"
#include "scipp/common/reduction.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/core/element/comparison.h"
#include "scipp/core/element/logical.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;
using scipp::common::reduce_all_dims;

namespace scipp::variable {

// Workaround VS C7526 (undefined inline variable) with dtype<> in template.
bool is_dtype_bool(const VariableConstView &var) {
  return var.dtype() == dtype<bool>;
}
bool is_dtype_int64(const VariableConstView &var) {
  return var.dtype() == dtype<int64_t>;
}

void sum_impl(const VariableView &summed, const VariableConstView &var) {
  accumulate_in_place(summed, var, element::plus_equals);
}

void nansum_impl(const VariableView &summed, const VariableConstView &var) {
  accumulate_in_place(summed, var, element::nan_plus_equals);
}

template <typename Op>
Variable sum_with_dim_impl(Op op, const VariableConstView &var, const Dim dim) {
  auto dims = var.dims();
  dims.erase(dim);
  // Bool DType is a bit special in that it cannot contain it's sum.
  // Instead the sum is stored in a int64_t Variable
  Variable summed{is_dtype_bool(var) ? makeVariable<int64_t>(Dimensions(dims))
                                     : Variable(var, dims)};
  op(summed, var);
  return summed;
}

template <typename Op>
VariableView sum_with_dim_inplace_impl(Op op, const VariableConstView &var,
                                       const Dim dim, const VariableView &out) {
  if (is_dtype_bool(var) && !is_dtype_int64(out))
    throw except::UnitError("In-place sum of Bool dtype must be stored in an "
                            "output variable of Int64 dtype.");

  auto dims = var.dims();
  dims.erase(dim);
  if (dims != out.dims())
    throw except::DimensionError(
        "Output argument dimensions must be equal to input dimensions without "
        "the summing dimension.");

  op(out, var);
  return out;
}

Variable sum(const VariableConstView &var, const Dim dim) {
  return sum_with_dim_impl(sum_impl, var, dim);
}

Variable nansum(const VariableConstView &var, const Dim dim) {
  return sum_with_dim_impl(nansum_impl, var, dim);
}

VariableView sum(const VariableConstView &var, const Dim dim,
                 const VariableView &out) {
  return sum_with_dim_inplace_impl(sum_impl, var, dim, out);
}

VariableView nansum(const VariableConstView &var, const Dim dim,
                    const VariableView &out) {
  return sum_with_dim_inplace_impl(nansum_impl, var, dim, out);
}

Variable make_scale(const VariableConstView &var, Dim dim,
                    const VariableConstView &masks_sum) {
  Variable countvar{var};
  countvar.setUnit(units::one);
  countvar *= 0 * units::one;
  countvar += 1 * units::one;
  auto count = nansum(countvar, dim); // Extents excluding nans
  return 1.0 * units::one / (count - masks_sum);
}

Variable nanmean_impl(const VariableConstView &var, const Dim dim,
                      const VariableConstView &masks_sum) {
  auto summed = nanmean(var, dim);
  auto scale = make_scale(var, dim, masks_sum);
  if (isInt(var.dtype()))
    summed = summed * scale;
  else
    summed *= scale;
  return summed;
}

VariableView nanmean_impl(const VariableConstView &var, const Dim dim,
                          const VariableConstView &masks_sum,
                          const VariableView &out) {
  if (isInt(out.dtype()))
    throw except::UnitError(
        "Cannot calculate mean in-place when output dtype is integer");

  nanmean(var, dim, out);
  out *= make_scale(var, dim, masks_sum);
  return out;
}

Variable mean_impl(const VariableConstView &var, const Dim dim,
                   const VariableConstView &masks_sum) {
  auto summed = sum(var, dim);
  auto scale = 1.0 * units::one / (var.dims()[dim] * units::one - masks_sum);
  if (isInt(var.dtype()))
    summed = summed * scale;
  else
    summed *= scale;
  return summed;
}

VariableView mean_impl(const VariableConstView &var, const Dim dim,
                       const VariableConstView &masks_sum,
                       const VariableView &out) {
  if (isInt(out.dtype()))
    throw except::UnitError(
        "Cannot calculate mean in-place when output dtype is integer");

  sum(var, dim, out);
  out *= 1.0 * units::one / (var.dims()[dim] * units::one - masks_sum);
  return out;
}

/// Return the mean along all dimensions.
Variable mean(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return mean(_...); });
}

Variable mean(const VariableConstView &var, const Dim dim) {
  return mean_impl(var, dim, makeVariable<int64_t>(Values{0}));
}

VariableView mean(const VariableConstView &var, const Dim dim,
                  const VariableView &out) {
  return mean_impl(var, dim, makeVariable<int64_t>(Values{0}), out);
}

/// Return the mean along all dimensions. Ignoring NaN values.
Variable nanmean(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nanmean(_...); });
}

Variable nanmean(const VariableConstView &var, const Dim dim) {
  return nanmean_impl(var, dim, makeVariable<int64_t>(Values{0}));
}

VariableView nanmean(const VariableConstView &var, const Dim dim,
                     const VariableView &out) {
  return nanmean_impl(var, dim, makeVariable<int64_t>(Values{0}), out);
}

template <class Op>
void reduce_impl(const VariableView &out, const VariableConstView &var, Op op) {
  accumulate_in_place(out, var, op);
}

/// Reduction for idempotent operations such that op(a,a) = a.
///
/// The requirement for idempotency comes from the way the reduction output is
/// initialized. It is fulfilled for operations like `or`, `and`, `min`, and
/// `max`. Note that masking is not supported here since it would make creation
/// of a sensible starting value difficult.
template <class Op>
Variable reduce_idempotent(const VariableConstView &var, const Dim dim, Op op) {
  Variable out(var.slice({dim, 0}));
  reduce_impl(out, var, op);
  return out;
}

void any_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl(out, var, core::element::logical_or_equals);
}

Variable any(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::logical_or_equals);
}

void all_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl(out, var, core::element::logical_and_equals);
}

Variable all(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::logical_and_equals);
}

void max_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl(out, var, core::element::max_equals);
}

/// Return the maximum along given dimension.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable max(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::max_equals);
}

/// Return the maximum along given dimension ignoring NaN values.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable nanmax(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::nanmax_equals);
}

void min_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl(out, var, core::element::min_equals);
}

/// Return the minimum along given dimension.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable min(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::min_equals);
}

/// Return the minimum along given dimension ignorning NaN values.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable nanmin(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::nanmin_equals);
}

/// Return the sum along all dimensions.
Variable sum(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return sum(_...); });
}

/// Return the sum along all dimensions, nans treated as zero.
Variable nansum(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nansum(_...); });
}

/// Return the maximum along all dimensions.
Variable max(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return max(_...); });
}

/// Return the maximum along all dimensions ignorning NaN values.
Variable nanmax(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nanmax(_...); });
}

/// Return the minimum along all dimensions.
Variable min(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return min(_...); });
}

/// Return the minimum along all dimensions ignoring NaN values.
Variable nanmin(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return nanmin(_...); });
}

/// Return the logical AND along all dimensions.
Variable all(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return all(_...); });
}

/// Return the logical OR along all dimensions.
Variable any(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return any(_...); });
}

} // namespace scipp::variable
