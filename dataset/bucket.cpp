// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/common/overloaded.h"
#include "scipp/core/bucket.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/except.h"
#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"


namespace scipp::dataset::buckets {

namespace {

auto sizes_to_begin(const VariableConstView &sizes) {
  Variable begin(sizes);
  scipp::index size = 0;
  for (auto &i : begin.values<scipp::index>()) {
    const auto old_size = size;
    size += i;
    i = old_size;
  }
  return std::tuple{begin, size};
}

constexpr auto copy_spans = overloaded{
    core::element::arg_list<std::tuple<span<double>, span<const double>>>,
    core::transform_flags::expect_all_or_none_have_variance,
    [](units::Unit &a, const units::Unit &b) { core::expect::equals(a, b); },
    [](auto &dst, const auto &src) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(dst)>>) {
        std::copy(src.value.begin(), src.value.end(), dst.value.begin());
        std::copy(src.variance.begin(), src.variance.end(),
                  dst.variance.begin());
      } else {
        std::copy(src.begin(), src.end(), dst.begin());
      }
    }};

void copy(const VariableConstView &src, const VariableView &dst, const Dim dim,
          const VariableConstView &srcIndices,
          const VariableConstView &dstIndices) {
  transform_in_place(subspan_view(dst, dim, dstIndices),
                     subspan_view(src, dim, srcIndices), copy_spans);
}

void copy(const DataArrayConstView &src, const DataArrayView &dst,
          const Dim dim, const VariableConstView &srcIndices,
          const VariableConstView &dstIndices) {
  copy(src.data(), dst.data(), dim, srcIndices, dstIndices);
  const auto copy_or_match = [&](const auto &a, const auto &b) {
    if (a.dims().contains(dim))
      copy(a, b, dim, srcIndices, dstIndices);
    else
      core::expect::equals(a, b);
  };
  for (const auto &[name, coord] : src.coords())
    copy_or_match(coord, dst.coords()[name]);
  for (const auto &[name, mask] : src.masks())
    copy_or_match(mask, dst.masks()[name]);
}

auto resize_buffer(const VariableConstView &parent, const Dim dim,
                   const scipp::index size) {
  return resize(parent, dim, size);
}

auto resize_buffer(const DataArrayConstView &parent, const Dim dim,
                   const scipp::index size) {
  const auto copy_or_resize = [dim, size](const auto &var) {
    // TODO Could avoid init here for better performance.
    return var.dims().contains(dim) ? resize(var, dim, size) : Variable(var);
  };
  DataArray buffer(resize(parent.data(), dim, size));
  for (const auto &[name, var] : parent.aligned_coords())
    buffer.aligned_coords().set(name, copy_or_resize(var));
  for (const auto &[name, var] : parent.masks())
    buffer.masks().set(name, copy_or_resize(var));
  for (const auto &[name, var] : parent.unaligned_coords())
    buffer.unaligned_coords().set(name, copy_or_resize(var));
  return buffer;
}

template <class T>
auto combine(const VariableConstView &var0, const VariableConstView &var1) {
  const auto &[indices0, dim0, buffer0] = var0.constituents<bucket<T>>();
  const auto &[indices1, dim1, buffer1] = var1.constituents<bucket<T>>();
  const Dim dim = dim0;
  const auto [begin0, end0] = unzip(indices0);
  const auto [begin1, end1] = unzip(indices1);
  const auto sizes0 = end0 - begin0;
  const auto sizes1 = end1 - begin1;
  const auto sizes = sizes0 + sizes1;
  const auto [begin, size] = sizes_to_begin(sizes);
  const auto end = begin + sizes;
  auto buffer = resize_buffer(buffer0, dim, size);
  copy(buffer0, buffer, dim, indices0, zip(begin, end - sizes1));
  copy(buffer1, buffer, dim, indices1, zip(begin + sizes0, end));
  return variable::DataModel<bucket<T>>{zip(begin, end), dim,
                                        std::move(buffer)};
}

template <class T>
auto concatenate_impl(const VariableConstView &var0,
                      const VariableConstView &var1) {
  return Variable{
      std::make_unique<variable::DataModel<bucket<T>>>(combine<T>(var0, var1))};
}
} // namespace

Variable concatenate(const VariableConstView &var0,
                     const VariableConstView &var1) {
  if (var0.dtype() == dtype<bucket<Variable>>)
    return concatenate_impl<Variable>(var0, var1);
  else
    return concatenate_impl<DataArray>(var0, var1);
}

void append(const VariableView &var0, const VariableConstView &var1) {
  if (var0.dtype() == dtype<bucket<Variable>>)
    var0.replace_model(combine<Variable>(var0, var1));
  else
    var0.replace_model(combine<DataArray>(var0, var1));
}

} // namespace scipp::dataset::buckets
