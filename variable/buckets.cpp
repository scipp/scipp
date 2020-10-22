// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/buckets.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

namespace scipp::variable {

namespace {
constexpr auto copy_spans = overloaded{
    core::element::arg_list<std::tuple<span<double>, span<const double>>>,
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
}

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

} // namespace scipp::variable
