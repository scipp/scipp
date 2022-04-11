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
#include "scipp/variable/math.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {

// Workaround VS C7526 (undefined inline variable) with dtype<> in template.
bool is_dtype_bool(const Variable &var) { return var.dtype() == dtype<bool>; }

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

} // namespace

void sum_impl(Variable &summed, const Variable &var) {
  if (summed.dtype() == dtype<float>) {
    auto accum = astype(summed, dtype<double>);
    sum_impl(accum, var);
    copy(astype(accum, dtype<float>), summed);
  } else {
    accumulate_in_place(summed, var, element::add_equals, "sum");
  }
}

void nansum_impl(Variable &summed, const Variable &var) {
  if (summed.dtype() == dtype<float>) {
    auto accum = astype(summed, dtype<double>);
    nansum_impl(accum, var);
    copy(astype(accum, dtype<float>), summed);
  } else {
    accumulate_in_place(summed, var, element::nan_add_equals, "nansum");
  }
}

template <typename Op>
Variable sum_with_dim_impl(Op op, const Variable &var, const Dim dim) {
  // Bool DType is a bit special in that it cannot contain its sum.
  // Instead the sum is stored in a int64_t Variable
  auto summed = make_accumulant(var, dim, FillValue::ZeroNotBool);
  op(summed, var);
  return summed;
}

template <typename Op>
Variable &sum_with_dim_inplace_impl(Op op, const Variable &var, const Dim dim,
                                    Variable &out) {
  if (is_dtype_bool(var) && is_dtype_bool(out))
    throw except::TypeError("In-place sum of dtype=bool cannot be stored in an "
                            "output variable with dtype=bool.");

  auto dims = var.dims();
  dims.erase(dim);
  if (dims != out.dims())
    throw except::DimensionError(
        "Output argument dimensions must be equal to input dimensions without "
        "the summing dimension.");

  out.setUnit(var.unit());
  op(out, var);
  return out;
}

Variable sum(const Variable &var, const Dim dim) {
  return sum_with_dim_impl(sum_impl, var, dim);
}

Variable nansum(const Variable &var, const Dim dim) {
  return sum_with_dim_impl(nansum_impl, var, dim);
}

Variable &sum(const Variable &var, const Dim dim, Variable &out) {
  return sum_with_dim_inplace_impl(sum_impl, var, dim, out);
}

Variable &nansum(const Variable &var, const Dim dim, Variable &out) {
  return sum_with_dim_inplace_impl(nansum_impl, var, dim, out);
}

Variable mean_impl(const Variable &var, const Dim dim, const Variable &count) {
  return normalize_impl(sum(var, dim), count);
}

Variable nanmean_impl(const Variable &var, const Dim dim,
                      const Variable &count) {
  return normalize_impl(nansum(var, dim), count);
}

Variable &mean_impl(const Variable &var, const Dim dim, const Variable &count,
                    Variable &out) {
  if (is_int(out.dtype()))
    throw except::TypeError(
        "Cannot calculate mean in-place when output dtype is integer");
  sum(var, dim, out);
  normalize_inplace_impl(out, count);
  return out;
}

Variable &nanmean_impl(const Variable &var, const Dim dim,
                       const Variable &count, Variable &out) {
  if (is_int(out.dtype()))
    throw except::TypeError(
        "Cannot calculate nanmean in-place when output dtype is integer");
  nansum(var, dim, out);
  normalize_inplace_impl(out, count);
  return out;
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

/// Return the mean along all dimensions.
Variable mean(const Variable &var) {
  return normalize_impl(sum(var), count(var));
}

Variable mean(const Variable &var, const Dim dim) {
  return mean_impl(var, dim, count(var, dim));
}

Variable &mean(const Variable &var, const Dim dim, Variable &out) {
  return mean_impl(var, dim, count(var, dim), out);
}

/// Return the mean along all dimensions. Ignoring NaN values.
Variable nanmean(const Variable &var) {
  return normalize_impl(nansum(var), sum(isfinite(var)));
}

Variable nanmean(const Variable &var, const Dim dim) {
  return nanmean_impl(var, dim, sum(isfinite(var), dim));
}

Variable &nanmean(const Variable &var, const Dim dim, Variable &out) {
  return nanmean_impl(var, dim, sum(isfinite(var), dim), out);
}

template <class Op>
void reduce_impl(Variable &out, const Variable &var, Op op,
                 const std::string_view name) {
  accumulate_in_place(out, var, op, name);
}

/// Reduction for idempotent operations such that op(a,a) = a.
///
/// The requirement for idempotency comes from the way the reduction output is
/// initialized. It is fulfilled for operations like `or`, `and`, `min`, and
/// `max`. Note that masking is not supported here since it would make creation
/// of a sensible starting value difficult.
template <class Op>
Variable reduce_idempotent(const Variable &var, const Dim dim, Op op,
                           const FillValue &init, const std::string_view name) {
  auto out = make_accumulant(var, dim, init);
  reduce_impl(out, var, op, name);
  return out;
}

void any_impl(Variable &out, const Variable &var) {
  reduce_impl(out, var, core::element::logical_or_equals, "any");
}

Variable any(const Variable &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::logical_or_equals,
                           FillValue::False, "any");
}

void all_impl(Variable &out, const Variable &var) {
  reduce_impl(out, var, core::element::logical_and_equals, "all");
}

Variable all(const Variable &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::logical_and_equals,
                           FillValue::True, "all");
}

void max_impl(Variable &out, const Variable &var) {
  reduce_impl(out, var, core::element::max_equals, "max");
}

/// Return the maximum along given dimension.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable max(const Variable &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::max_equals,
                           FillValue::Lowest, "max");
}

/// Return the maximum along given dimension ignoring NaN values.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable nanmax(const Variable &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::nanmax_equals,
                           FillValue::Lowest, "nanmax");
}

void min_impl(Variable &out, const Variable &var) {
  reduce_impl(out, var, core::element::min_equals, "min");
}

/// Return the minimum along given dimension.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable min(const Variable &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::min_equals, FillValue::Max,
                           "min");
}

/// Return the minimum along given dimension ignorning NaN values.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable nanmin(const Variable &var, const Dim dim) {
  return reduce_idempotent(var, dim, core::element::nanmin_equals,
                           FillValue::Max, "nanmin");
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

} // namespace scipp::variable
