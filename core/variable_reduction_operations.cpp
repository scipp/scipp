// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include "operators.h"
#include "variable_operations_common.h"

namespace scipp::core {

namespace sparse {
/// Return array of sparse dimension extents, i.e., total counts.
Variable counts(const VariableConstProxy &var) {
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
void reserve(const VariableProxy &sparse, const VariableConstProxy &capacity) {
  transform_in_place<
      pair_custom_t<std::pair<sparse_container<double>, scipp::index>>,
      pair_custom_t<std::pair<sparse_container<float>, scipp::index>>,
      pair_custom_t<std::pair<sparse_container<int64_t>, scipp::index>>,
      pair_custom_t<std::pair<sparse_container<int32_t>, scipp::index>>>(
      sparse, capacity,
      overloaded{[](auto &&sparse_, const scipp::index capacity_) {
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

void flatten_impl(const VariableProxy &summed, const VariableConstProxy &var,
                  const Variable &mask) {
  // Note that mask may often be "empty" (0-D false). Benchmarks show no
  // significant penalty from handling it anyway. We thus avoid two separate
  // code branches here.
  if (!var.dims().sparse())
    throw except::DimensionError("`flatten` can only be used for sparse data, "
                                 "use `sum` for dense data.");
  // 1. Reserve space in output. This yields approx. 3x speedup.
  auto summed_counts = sparse::counts(summed);
  sum_impl(summed_counts, sparse::counts(var) * ~mask);
  sparse::reserve(summed, summed_counts);

  // 2. Flatten dimension(s) by concatenating along sparse dim.
  using namespace flatten_detail;
  accumulate_in_place<
      std::tuple<args<double>, args<float>, args<int64_t>, args<int32_t>>>(
      summed, var, mask,
      overloaded{
          [](auto &a, const auto &b, const auto &mask_) {
            if (!mask_)
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
Variable flatten(const VariableConstProxy &var, const Dim dim) {
  auto dims = var.dims();
  dims.erase(dim);
  Variable flattened(var, dims);
  flatten_impl(flattened, var);
  return flattened;
}

/// Flatten with mask, skipping masked elements.
Variable flatten(const VariableConstProxy &var, const Dim dim,
                 const MasksConstProxy &masks) {
  auto dims = var.dims();
  dims.erase(dim);
  Variable flattened(var, dims);
  const auto mask = masks_merge_if_contains(masks, dim);
  flatten_impl(flattened, var, mask);
  return flattened;
}

void sum_impl(const VariableProxy &summed, const VariableConstProxy &var) {
  if (var.dims().sparse())
    throw except::DimensionError("`sum` can only be used for dense data, use "
                                 "`flatten` for sparse data.");
  accumulate_in_place<
      pair_self_t<double, float, int64_t, int32_t, Eigen::Vector3d>,
      pair_custom_t<std::pair<int64_t, bool>>>(
      summed, var, [](auto &&a, auto &&b) { a += b; });
}

Variable sum(const VariableConstProxy &var, const Dim dim) {
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

VariableProxy sum(const VariableConstProxy &var, const Dim dim,
                  const VariableProxy &out) {
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

Variable sum(const VariableConstProxy &var, const Dim dim,
             const MasksConstProxy &masks) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim))
      return sum(var * ~mask_union, dim);
  }
  return sum(var, dim);
}

VariableProxy sum(const VariableConstProxy &var, const Dim dim,
                  const MasksConstProxy &masks, const VariableProxy &out) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim))
      return sum(var * ~mask_union, dim, out);
  }
  return sum(var, dim, out);
}

Variable mean(const VariableConstProxy &var, const Dim dim,
              const VariableConstProxy &masks_sum) {
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

VariableProxy mean(const VariableConstProxy &var, const Dim dim,
                   const VariableConstProxy &masks_sum,
                   const VariableProxy &out) {
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

Variable mean(const VariableConstProxy &var, const Dim dim) {
  return mean(var, dim, makeVariable<int64_t>(Values{0}));
}

VariableProxy mean(const VariableConstProxy &var, const Dim dim,
                   const VariableProxy &out) {
  return mean(var, dim, makeVariable<int64_t>(Values{0}), out);
}

Variable mean(const VariableConstProxy &var, const Dim dim,
              const MasksConstProxy &masks) {
  if (!masks.empty()) {
    const auto mask_union = masks_merge_if_contains(masks, dim);
    if (mask_union.dims().contains(dim)) {
      const auto masks_sum = sum(mask_union, dim);
      return mean(var * ~mask_union, dim, masks_sum);
    }
  }
  return mean(var, dim);
}

VariableProxy mean(const VariableConstProxy &var, const Dim dim,
                   const MasksConstProxy &masks, const VariableProxy &out) {
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
void reduce_impl(const VariableProxy &out, const VariableConstProxy &var) {
  expect::notSparse(var);
  accumulate_in_place(out, var, Op{});
}

template <class Op>
Variable reduce(const VariableConstProxy &var, const Dim dim) {
  Variable out(var.slice({dim, 0}));
  reduce_impl<Op>(out, var);
  return out;
}

void any_impl(const VariableProxy &out, const VariableConstProxy &var) {
  reduce_impl<operator_detail::or_equals>(out, var);
}

Variable any(const VariableConstProxy &var, const Dim dim) {
  return reduce<operator_detail::or_equals>(var, dim);
}

void all_impl(const VariableProxy &out, const VariableConstProxy &var) {
  reduce_impl<operator_detail::and_equals>(out, var);
}

Variable all(const VariableConstProxy &var, const Dim dim) {
  return reduce<operator_detail::and_equals>(var, dim);
}

/// Return the maximum along given dimension.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable max(const VariableConstProxy &var, const Dim dim) {
  return reduce<operator_detail::max_equals>(var, dim);
}

/// Return the minimum along given dimension.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable min(const VariableConstProxy &var, const Dim dim) {
  return reduce<operator_detail::min_equals>(var, dim);
}

} // namespace scipp::core
