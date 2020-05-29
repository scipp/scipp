// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/reduction.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element/arithmetic.h"
#include "scipp/core/element/comparison.h"
#include "scipp/core/element/logical.h"
#include "scipp/core/element/reduction.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/event.h"
#include "scipp/variable/except.h"
#include "scipp/variable/transform.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

void flatten_impl(const VariableView &summed, const VariableConstView &var,
                  const VariableConstView &mask) {
  // Note that mask may often be "empty" (0-D false). Benchmarks show no
  // significant penalty from handling it anyway. We thus avoid two separate
  // code branches here.
  if (!contains_events(var))
    throw except::TypeError("`flatten` can only be used for event data, "
                            "use `sum` for dense data.");
  // 1. Reserve space in output. This yields approx. 3x speedup.
  auto summed_sizes = event::sizes(summed);
  sum_impl(summed_sizes, event::sizes(var) * mask);
  event::reserve(summed, summed_sizes);

  // 2. Flatten dimension(s) by concatenating along events dim.
  accumulate_in_place(summed, var, mask, element::flatten);
}

/// Flatten dimension by concatenating along events dimension.
///
/// This is equivalent to summing dense data along a dimension, in the sense
/// that summing histogrammed data is the same as histogramming flattened data.
Variable flatten(const VariableConstView &var, const Dim dim) {
  auto dims = var.dims();
  dims.erase(dim);
  Variable flattened(var, dims);
  flatten_impl(flattened, var, makeVariable<bool>(Values{true}));
  return flattened;
}

void sum_impl(const VariableView &summed, const VariableConstView &var) {
  if (contains_events(var))
    throw except::TypeError("`sum` can only be used for dense data, use "
                            "`flatten` for event data.");
  accumulate_in_place(summed, var, element::plus_equals);
}

Variable sum(const VariableConstView &var, const Dim dim) {
  auto dims = var.dims();
  dims.erase(dim);
  // Bool DType is a bit special in that it cannot contain it's sum.
  // Instead the sum is stored in a int64_t Variable
  Variable summed{var.dtype() == dtype<bool>
                      ? makeVariable<int64_t>(Dimensions(dims))
                      : Variable(var, dims)};
  sum_impl(summed, var);
  return summed;
}

VariableView sum(const VariableConstView &var, const Dim dim,
                 const VariableView &out) {
  if (var.dtype() == dtype<bool> && out.dtype() != dtype<int64_t>)
    throw except::UnitError("In-place sum of Bool dtype must be stored in an "
                            "output variable of Int64 dtype.");

  auto dims = var.dims();
  dims.erase(dim);
  if (dims != out.dims())
    throw except::DimensionError(
        "Output argument dimensions must be equal to input dimensions without "
        "the summing dimension.");

  sum_impl(out, var);
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

Variable mean(const VariableConstView &var, const Dim dim) {
  return mean_impl(var, dim, makeVariable<int64_t>(Values{0}));
}

VariableView mean(const VariableConstView &var, const Dim dim,
                  const VariableView &out) {
  return mean_impl(var, dim, makeVariable<int64_t>(Values{0}), out);
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

/// Return the maximum along all dimensions.
Variable max(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return max(_...); });
}

/// Return the minimum along all dimensions.
Variable min(const VariableConstView &var) {
  return reduce_all_dims(var, [](auto &&... _) { return min(_...); });
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
