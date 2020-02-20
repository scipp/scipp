// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"
#include "scipp/core/view_decl.h"

#include "operators.h"
#include "variable_operations_common.h"

namespace scipp::core {

namespace sparse {
/// Return array of sparse dimension extents, i.e., total counts.
Variable counts(const VariableConstView &var) {
  // To simplify this we would like to use `transform`, but this is currently
  // not possible since the current implementation expects outputs with
  // variances if any of the inputs has variances.
  auto dims = var.dims();
  dims.erase(dims.sparseDim());
  auto counts =
      makeVariable<scipp::index>(Dimensions(dims), units::Unit(units::counts));
  accumulate_in_place<
      pair_custom_t<std::pair<scipp::index, sparse_container<double>>>,
      pair_custom_t<std::pair<scipp::index, sparse_container<float>>>,
      pair_custom_t<std::pair<scipp::index, sparse_container<int64_t>>>,
      pair_custom_t<std::pair<scipp::index, sparse_container<int32_t>>>>(
      counts, var,
      overloaded{[](scipp::index &c, const auto &sparse) { c = sparse.size(); },
                 transform_flags::expect_no_variance_arg<0>});
  return counts;
}

/// Reserve memory in all sparse containers in `sparse`, based on `capacity`.
///
/// To avoid pessimizing reserves, this does nothing if the new capacity is less
/// than the typical logarithmic growth. This yields a 5x speedup in some cases,
/// without apparent negative effect on the other cases.
void reserve(const VariableView &sparse, const VariableConstView &capacity) {
  transform_in_place<
      pair_custom_t<std::pair<sparse_container<double>, scipp::index>>,
      pair_custom_t<std::pair<sparse_container<float>, scipp::index>>,
      pair_custom_t<std::pair<sparse_container<int64_t>, scipp::index>>,
      pair_custom_t<std::pair<sparse_container<int32_t>, scipp::index>>>(
      sparse, capacity,
      overloaded{[](auto &&sparse_, const scipp::index capacity_) {
                   if (capacity_ > 2 * scipp::size(sparse_))
                     return sparse_.reserve(capacity_);
                 },
                 transform_flags::expect_no_variance_arg<1>,
                 [](const units::Unit &, const units::Unit &) {}});
}
} // namespace sparse

namespace flatten_detail {
template <class T>
using args = std::tuple<sparse_container<T>, sparse_container<T>, bool>;
}

void flatten_impl(const VariableView &summed, const VariableConstView &var,
                  const VariableConstView &mask) {
  // Note that mask may often be "empty" (0-D false). Benchmarks show no
  // significant penalty from handling it anyway. We thus avoid two separate
  // code branches here.
  if (!var.dims().sparse())
    throw except::DimensionError("`flatten` can only be used for sparse data, "
                                 "use `sum` for dense data.");
  // 1. Reserve space in output. This yields approx. 3x speedup.
  auto summed_counts = sparse::counts(summed);
  sum_impl(summed_counts, sparse::counts(var) * mask);
  sparse::reserve(summed, summed_counts);

  // 2. Flatten dimension(s) by concatenating along sparse dim.
  using namespace flatten_detail;
  accumulate_in_place<
      std::tuple<args<double>, args<float>, args<int64_t>, args<int32_t>>>(
      summed, var, mask,
      overloaded{
          [](auto &a, const auto &b, const auto &mask_) {
            if (mask_)
              a.insert(a.end(), b.begin(), b.end());
          },
          [](units::Unit &a, const units::Unit &b, const units::Unit &mask_) {
            expect::equals(mask_, units::dimensionless);
            expect::equals(a, b);
          }});
}

/// Flatten dimension by concatenating along sparse dimension.
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

/// Flatten with mask, skipping masked elements.
Variable flatten(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks) {
  auto dims = var.dims();
  dims.erase(dim);
  Variable flattened(var, dims);
  const auto mask = ~masks_merge_if_contains(masks, dim);
  flatten_impl(flattened, var, mask);
  return flattened;
}

void sum_impl(const VariableView &summed, const VariableConstView &var) {
  if (var.dims().sparse())
    throw except::DimensionError("`sum` can only be used for dense data, use "
                                 "`flatten` for sparse data.");
  accumulate_in_place<
      pair_self_t<double, float, int64_t, int32_t, Eigen::Vector3d>,
      pair_custom_t<std::pair<int64_t, bool>>>(
      summed, var, [](auto &&a, auto &&b) { a += b; });
}

Variable sum(const VariableConstView &var, const Dim dim) {
  auto dims = var.dims();
  dims.erase(dim);
  // Bool DType is a bit special in that it cannot contain it's sum.
  // Instead the sum is stored in a int64_t Variable
  Variable summed{var.dtype() == DType::Bool
                      ? makeVariable<int64_t>(Dimensions(dims))
                      : Variable(var, dims)};
  sum_impl(summed, var);
  return summed;
}

VariableView sum(const VariableConstView &var, const Dim dim,
                 const VariableView &out) {
  if (var.dtype() == DType::Bool && out.dtype() != DType::Int64)
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

Variable sum(const VariableConstView &var, const Dim dim,
             const MasksConstView &masks) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim))
      return sum(var * ~mask_union, dim);
  }
  return sum(var, dim);
}

VariableView sum(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks, const VariableView &out) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim))
      return sum(var * ~mask_union, dim, out);
  }
  return sum(var, dim, out);
}

Variable mean(const VariableConstView &var, const Dim dim,
              const VariableConstView &masks_sum) {
  // In principle we *could* support mean/sum over sparse dimension.
  expect::notSparse(var);
  auto summed = sum(var, dim);

  auto scale =
      1.0 / (makeVariable<double>(Values{var.dims()[dim]}) - masks_sum);

  if (isInt(var.dtype()))
    summed = summed * scale;
  else
    summed *= scale;
  return summed;
}

VariableView mean(const VariableConstView &var, const Dim dim,
                  const VariableConstView &masks_sum, const VariableView &out) {
  // In principle we *could* support mean/sum over sparse dimension.
  expect::notSparse(var);
  if (isInt(out.dtype()))
    throw except::UnitError(
        "Cannot calculate mean in-place when output dtype is integer");

  sum(var, dim, out);

  auto scale =
      1.0 / (makeVariable<double>(Values{var.dims()[dim]}) - masks_sum);

  out *= scale;
  return out;
}

Variable mean(const VariableConstView &var, const Dim dim) {
  return mean(var, dim, makeVariable<int64_t>(Values{0}));
}

VariableView mean(const VariableConstView &var, const Dim dim,
                  const VariableView &out) {
  return mean(var, dim, makeVariable<int64_t>(Values{0}), out);
}

Variable mean(const VariableConstView &var, const Dim dim,
              const MasksConstView &masks) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim)) {
      const auto masks_sum = sum(mask_union, dim);
      return mean(var * ~mask_union, dim, masks_sum);
    }
  }
  return mean(var, dim);
}

VariableView mean(const VariableConstView &var, const Dim dim,
                  const MasksConstView &masks, const VariableView &out) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim)) {
      const auto masks_sum = sum(mask_union, dim);
      return mean(var * ~mask_union, dim, masks_sum, out);
    }
  }
  return mean(var, dim, out);
}

template <class Op>
void reduce_impl(const VariableView &out, const VariableConstView &var) {
  expect::notSparse(var);
  accumulate_in_place(out, var, Op{});
}

/// Reduction for idempotent operations such that op(a,a) = a.
///
/// The requirement for idempotency comes from the way the reduction output is
/// initialized. It is fulfilled for operations like `or`, `and`, `min`, and
/// `max`. Note that masking is not supported here since it would make creation
/// of a sensible starting value difficult.
template <class Op>
Variable reduce_idempotent(const VariableConstView &var, const Dim dim) {
  Variable out(var.slice({dim, 0}));
  reduce_impl<Op>(out, var);
  return out;
}

void any_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl<operator_detail::or_equals>(out, var);
}

Variable any(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent<operator_detail::or_equals>(var, dim);
}

void all_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl<operator_detail::and_equals>(out, var);
}

Variable all(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent<operator_detail::and_equals>(var, dim);
}

void max_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl<operator_detail::max_equals>(out, var);
}

/// Return the maximum along given dimension.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable max(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent<operator_detail::max_equals>(var, dim);
}

void min_impl(const VariableView &out, const VariableConstView &var) {
  reduce_impl<operator_detail::min_equals>(out, var);
}

/// Return the minimum along given dimension.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable min(const VariableConstView &var, const Dim dim) {
  return reduce_idempotent<operator_detail::min_equals>(var, dim);
}

/// Merges all masks contained in the MasksConstView that have the supplied
//  dimension in their dimensions into a single Variable
Variable masks_merge_if_contains(const MasksConstView &masks, const Dim dim) {
  auto mask_union = makeVariable<bool>(Values{false});
  for (const auto &mask : masks) {
    if (mask.second.dims().contains(dim)) {
      mask_union = mask_union | mask.second;
    }
  }
  return mask_union;
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

} // namespace scipp::core
