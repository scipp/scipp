// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/except.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_concept.h"

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

void copy_slices(const Variable &src, Variable dst, const Dim dim,
                 const Variable &srcIndices, const Variable &dstIndices) {
  const auto [begin0, end0] = unzip(srcIndices);
  const auto [begin1, end1] = unzip(dstIndices);
  const auto sizes0 = end0 - begin0;
  const auto sizes1 = end1 - begin1;
  core::expect::equals(src.unit(), dst.unit());
  // May broadcast `src` but not `dst` since that would result in
  // multiple/conflicting writes to same bucket.
  expect::contains(sizes1.dims(), sizes0.dims());
  core::expect::equals(all(equal(sizes0, sizes1)),
                       makeVariable<bool>(Values{true}));
  transform_in_place(subspan_view(dst, dim, dstIndices),
                     subspan_view(src, dim, srcIndices), copy_spans);
}

Variable resize_default_init(const Variable &var, const Dim dim,
                             const scipp::index size) {
  auto dims = var.dims();
  if (dims.contains(dim))
    dims.resize(dim, size);
  // Using variableFactory instead of variable::resize for creating
  // _uninitialized_ variable.
  return variable::variableFactory().create(var.dtype(), dims, var.unit(),
                                            var.hasVariances());
}

/// Construct a bin-variable over a variable.
///
/// Each bin is represented by a VariableView. `indices` defines the array of
/// bins as slices of `buffer` along `dim`.
Variable make_bins(Variable indices, const Dim dim, Variable buffer) {
  expect_valid_bin_indices(indices.data_handle(), dim, buffer.dims());
  return make_bins_no_validate(std::move(indices), dim, buffer);
}

/// Construct a bin-variable over a variable without index validation.
///
/// Must be used only when it is guaranteed that indices are valid or overlap of
/// bins is acceptable.
Variable make_bins_no_validate(Variable indices, const Dim dim,
                               Variable buffer) {
  indices.setDataHandle(std::make_unique<variable::DataModel<bucket<Variable>>>(
      indices.data_handle(), dim, std::move(buffer)));
  return indices;
}

} // namespace scipp::variable
