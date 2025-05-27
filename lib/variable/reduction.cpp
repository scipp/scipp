// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
#include "scipp/variable/bins.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "operations_common.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {
Variable reduce_to_dims(const Variable &var, const Dimensions &target_dims,
                        void (*const op)(Variable &, const Variable &),
                        const FillValue init) {
  auto accum = dense_special_like(var, target_dims, init);
  // FillValue::ZeroNotBool is not allowed here because it produces a different
  // dtype from var (if var has dtype bool). ZeroNotBool and Default are
  // semantically equivalent apart from the dtype change. So it can be
  // substituted here.
  op(accum,
     variableFactory().apply_event_masks(
         var, (init == FillValue::ZeroNotBool) ? FillValue::Default : init));
  return accum;
}

Variable reduce_dim(const Variable &var, const Dim dim,
                    void (*const op)(Variable &, const Variable &),
                    const FillValue init) {
  auto dims = var.dims();
  if (dim != Dim::Invalid)
    dims.erase(dim);
  return reduce_to_dims(var, dims, op, init);
}

Variable reduce_bins(const Variable &data,
                     void (*const op)(Variable &, const Variable &),
                     const FillValue init) {
  return reduce_to_dims(data, data.dims(), op, init);
}
} // namespace

Variable sum(const Variable &var, const Dim dim) {
  // Bool DType is a bit special in that it cannot contain its sum.
  // Instead, the sum is stored in an int64_t Variable
  return reduce_dim(var, dim, sum_into, FillValue::ZeroNotBool);
}

Variable nansum(const Variable &var, const Dim dim) {
  // Bool DType is a bit special in that it cannot contain its sum.
  // Instead, the sum is stored in an int64_t Variable
  return reduce_dim(var, dim, nansum_into, FillValue::ZeroNotBool);
}

Variable any(const Variable &var, const Dim dim) {
  return reduce_dim(var, dim, any_into, FillValue::False);
}

Variable all(const Variable &var, const Dim dim) {
  return reduce_dim(var, dim, all_into, FillValue::True);
}

/// Return the maximum along given dimension.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable max(const Variable &var, const Dim dim) {
  return reduce_dim(var, dim, max_into, FillValue::Lowest);
}

/// Return the maximum along given dimension ignoring NaN values.
///
/// Variances are not considered when determining the maximum. If present, the
/// variance of the maximum element is returned.
Variable nanmax(const Variable &var, const Dim dim) {
  return reduce_dim(var, dim, nanmax_into, FillValue::Lowest);
}

/// Return the minimum along given dimension.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable min(const Variable &var, const Dim dim) {
  return reduce_dim(var, dim, min_into, FillValue::Max);
}

/// Return the minimum along given dimension ignoring NaN values.
///
/// Variances are not considered when determining the minimum. If present, the
/// variance of the minimum element is returned.
Variable nanmin(const Variable &var, const Dim dim) {
  return reduce_dim(var, dim, nanmin_into, FillValue::Max);
}

Variable mean_impl(const Variable &var, const Dim dim, const Variable &count) {
  return normalize_impl(sum(var, dim), count);
}

Variable nanmean_impl(const Variable &var, const Dim dim,
                      const Variable &count) {
  return normalize_impl(nansum(var, dim), count);
}

namespace {
Variable unmasked_events(const Variable &data) {
  if (const auto mask_union = variableFactory().irreducible_event_mask(data);
      mask_union.is_valid()) {
    // Trick to get the sizes of bins if masks are present - bin the masks
    // using the same dimension & indices as the data, and then sum the
    // inverse of the mask to get the number of unmasked entries.
    return make_bins_no_validate(data.bin_indices(),
                                 variableFactory().elem_dim(data), ~mask_union);
  }
  return {};
}

template <class... Dim> Variable count(const Variable &var, Dim &&...dim) {
  if (!is_bins(var)) {
    if constexpr (sizeof...(dim) == 0)
      return var.dims().volume() * sc_units::none;
    else
      return ((var.dims()[dim] * sc_units::none) * ...);
  }
  if (const auto unmasked = unmasked_events(var); unmasked.is_valid()) {
    return sum(unmasked, dim...);
  }
  const auto [begin, end] = unzip(var.bin_indices());
  return sum(end - begin, dim...);
}

Variable bins_count(const Variable &data) {
  if (const auto unmasked = unmasked_events(data); unmasked.is_valid()) {
    return bins_sum(unmasked);
  }
  return bin_sizes(data);
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
  return reduce_all_dims(var, [](auto &&..._) { return sum(_...); });
}

/// Return the sum along all dimensions, nans treated as zero.
Variable nansum(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return nansum(_...); });
}

/// Return the maximum along all dimensions.
Variable max(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return max(_...); });
}

/// Return the maximum along all dimensions ignoring NaN values.
Variable nanmax(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return nanmax(_...); });
}

/// Return the minimum along all dimensions.
Variable min(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return min(_...); });
}

/// Return the minimum along all dimensions ignoring NaN values.
Variable nanmin(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return nanmin(_...); });
}

/// Return the logical AND along all dimensions.
Variable all(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return all(_...); });
}

/// Return the logical OR along all dimensions.
Variable any(const Variable &var) {
  return reduce_all_dims(var, [](auto &&..._) { return any(_...); });
}

/// Return the mean along all dimensions.
Variable mean(const Variable &var) {
  return normalize_impl(sum(var), count(var));
}

/// Return the mean along all dimensions. Ignoring NaN values.
Variable nanmean(const Variable &var) {
  return normalize_impl(nansum(var), sum(isfinite(var)));
}

/// Return the sum of all events per bin.
Variable bins_sum(const Variable &data) {
  return reduce_bins(data, variable::sum_into, FillValue::ZeroNotBool);
}

/// Return the sum of all events per bin. Ignoring NaN values.
Variable bins_nansum(const Variable &data) {
  return reduce_bins(data, variable::nansum_into, FillValue::ZeroNotBool);
}

/// Return the maximum of all events per bin.
Variable bins_max(const Variable &data) {
  return reduce_bins(data, variable::max_into, FillValue::Lowest);
}

/// Return the maximum of all events per bin. Ignoring NaN values.
Variable bins_nanmax(const Variable &data) {
  return reduce_bins(data, variable::nanmax_into, FillValue::Lowest);
}

/// Return the minimum of all events per bin.
Variable bins_min(const Variable &data) {
  return reduce_bins(data, variable::min_into, FillValue::Max);
}

/// Return the minimum of all events per bin. Ignoring NaN values.
Variable bins_nanmin(const Variable &data) {
  return reduce_bins(data, variable::nanmin_into, FillValue::Max);
}

/// Return the logical AND of all events per bin.
Variable bins_all(const Variable &data) {
  return reduce_bins(data, variable::all_into, FillValue::True);
}

/// Return the logical OR of all events per bin.
Variable bins_any(const Variable &data) {
  return reduce_bins(data, variable::any_into, FillValue::False);
}

/// Return the mean of all events per bin.
Variable bins_mean(const Variable &data) {
  return normalize_impl(bins_sum(data), bins_count(data));
}

/// Return the mean of all events per bin. Ignoring NaN values.
Variable bins_nanmean(const Variable &data) {
  return normalize_impl(bins_nansum(data), bins_sum(isfinite(data)));
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
