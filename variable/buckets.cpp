// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/buckets.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

namespace scipp::variable {

namespace {
template <class T> using copy_spans_args = std::tuple<span<T>, span<const T>>;
constexpr auto copy_spans = overloaded{
    core::element::arg_list<copy_spans_args<double>, copy_spans_args<float>,
                            copy_spans_args<int64_t>, copy_spans_args<int32_t>,
                            copy_spans_args<bool>, copy_spans_args<std::string>,
                            copy_spans_args<core::time_point>>,
    core::transform_flags::expect_all_or_none_have_variance,
    [](units::Unit &a, const units::Unit &b) { a = b; },
    [](auto &dst, const auto &src) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(dst)>>) {
        std::copy(src.value.begin(), src.value.end(), dst.value.begin());
        std::copy(src.variance.begin(), src.variance.end(),
                  dst.variance.begin());
      } else {
        std::copy(src.begin(), src.end(), dst.begin());
      }
    }};
} // namespace

void copy_slices(const VariableConstView &src, const VariableView &dst,
                 const Dim dim, const VariableConstView &srcIndices,
                 const VariableConstView &dstIndices) {
  const auto [begin0, end0] = unzip(srcIndices);
  const auto [begin1, end1] = unzip(dstIndices);
  const auto sizes0 = end0 - begin0;
  const auto sizes1 = end1 - begin1;
  // May broadcast `src` but not `dst` since that would result in
  // multiple/conflicting writes to same bucket.
  expect::contains(sizes1.dims(), sizes0.dims());
  core::expect::equals(all(equal(sizes0, sizes1)),
                       makeVariable<bool>(Values{true}));
  transform_in_place(subspan_view(dst, dim, dstIndices),
                     subspan_view(src, dim, srcIndices), copy_spans);
}

Variable resize_default_init(const VariableConstView &var, const Dim dim,
                             const scipp::index size) {
  auto dims = var.dims();
  if (dims.contains(dim))
    dims.resize(dim, size);
  // Using variableFactory instead of variable::resize for creating
  // _uninitialized_ variable.
  return variable::variableFactory().create(var.dtype(), dims, var.unit(),
                                            var.hasVariances());
}

std::tuple<Variable, scipp::index>
sizes_to_begin(const VariableConstView &sizes) {
  Variable begin(sizes);
  scipp::index size = 0;
  for (auto &i : begin.values<scipp::index>()) {
    const auto old_size = size;
    size += i;
    i = old_size;
  }
  return {begin, size};
}

/// Construct a bin-variable over a variable.
///
/// Each bin is represented by a VariableView. `indices` defines the array of
/// bins as slices of `buffer` along `dim`.
Variable make_bins(Variable indices, const Dim dim, Variable buffer) {
  return {std::make_unique<variable::DataModel<bucket<Variable>>>(
      std::move(indices), dim, std::move(buffer))};
}

/// Construct non-owning binned variable of a mutable buffer.
///
/// This is intented for internal and short-lived variables. The returned
/// variable stores *views* onto `indices` and `buffer` rather than copying the
/// data. This is, it does not own any or share ownership of any data.
Variable make_non_owning_bins(const VariableConstView &indices, const Dim dim,
                              const VariableView &buffer) {
  return {std::make_unique<variable::DataModel<bucket<VariableView>>>(
      indices, dim, buffer)};
}

/// Construct non-owning binned variable of a const buffer.
///
/// This is intented for internal and short-lived variables. The returned
/// variable stores *views* onto `indices` and `buffer` rather than copying the
/// data. This is, it does not own any or share ownership of any data.
Variable make_non_owning_bins(const VariableConstView &indices, const Dim dim,
                              const VariableConstView &buffer) {
  return {std::make_unique<variable::DataModel<bucket<VariableConstView>>>(
      indices, dim, buffer)};
}

} // namespace scipp::variable
