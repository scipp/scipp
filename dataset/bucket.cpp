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

namespace scipp::core::element::bucket {

constexpr auto sizes =
    overloaded{element::arg_list<core::bucket_base::range_type>,
               transform_flags::expect_no_variance_arg<0>,
               [](const units::Unit &u) {
                 expect::equals(u, units::one);
                 return units::one;
               },
               [](const auto &range) { return range.second - range.first; }};

constexpr auto zip =
    overloaded{element::arg_list<scipp::index>,
               transform_flags::no_event_list_handling,
               transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>,
               [](const units::Unit &first, const units::Unit &second) {
                 expect::equals(first, second);
                 return first;
               },
               [](const auto first, const auto second) {
                 return std::pair{first, second};
               }};

constexpr auto add_to_component = overloaded{
    element::arg_list<std::tuple<core::bucket_base::range_type, int64_t>>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>,
    [](units::Unit &a, const units::Unit &b) { a += b; }};
constexpr auto add_to_first =
    overloaded{add_to_component, [](auto &a, const auto &b) { a.first += b; }};
constexpr auto add_to_second =
    overloaded{add_to_component, [](auto &a, const auto &b) { a.second += b; }};

constexpr auto copy = overloaded{
    element::arg_list<std::tuple<span<double>, span<const double>>>,
    transform_flags::expect_all_or_none_have_variance,
    [](units::Unit &a, const units::Unit &b) { expect::equals(a, b); },
    [](auto &dst, const auto &src) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(dst)>>) {
        std::copy(src.value.begin(), src.value.end(), dst.value.begin());
        std::copy(src.variance.begin(), src.variance.end(),
                  dst.variance.begin());
      } else {
        std::copy(src.begin(), src.end(), dst.begin());
      }
    }};
} // namespace scipp::core::element::bucket

namespace scipp::dataset {

namespace buckets {

/// TODO A number of these functions would be redundant if we used an inner
/// extra dim of size 2 instead of a pair.
auto sizes(const VariableConstView &indices) {
  return transform(indices, core::element::bucket::sizes);
}
auto zip(const VariableConstView &first, const VariableConstView &second) {
  return transform(first, second, core::element::bucket::zip);
}
void add_to_first(const VariableView &a, const VariableConstView &b) {
  transform_in_place(a, b, core::element::bucket::add_to_first);
}
void add_to_second(const VariableView &a, const VariableConstView &b) {
  transform_in_place(a, b, core::element::bucket::add_to_second);
}

auto sizes_to_indices(const VariableConstView &sizes) {
  auto indices = zip(sizes * makeVariable<scipp::index>(Values{0}), sizes);
  scipp::index size = 0;
  for (auto &range : indices.values<core::bucket_base::range_type>()) {
    range.second += size - range.first;
    range.first = size;
    size = range.second;
  }
  return std::tuple{indices, size};
}

void copy(const VariableConstView &src, const VariableView &dst, const Dim dim,
          const VariableConstView &srcIndices,
          const VariableConstView &dstIndices) {
  transform_in_place(subspan_view(dst, dim, dstIndices),
                     subspan_view(src, dim, srcIndices),
                     core::element::bucket::copy);
}

void copy(const DataArrayConstView &src, const DataArrayView &dst,
          const Dim dim, const VariableConstView &srcIndices,
          const VariableConstView &dstIndices) {
  copy(src.data(), dst.data(), dim, srcIndices, dstIndices);
  for (const auto &[name, coord] : src.coords())
    if (coord.dims().contains(dim))
      copy(coord, dst.coords()[name], dim, srcIndices, dstIndices);
  for (const auto &[name, mask] : src.masks())
    if (mask.dims().contains(dim))
      copy(mask, dst.masks()[name], dim, srcIndices, dstIndices);
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
  const auto &[ranges0, dim0, buffer0] = var0.constituents<bucket<T>>();
  const auto &[ranges1, dim1, buffer1] = var1.constituents<bucket<T>>();
  const Dim dim = dim0;
  const auto sizes0 = sizes(ranges0);
  const auto sizes1 = sizes(ranges1);
  const auto sizes = sizes0 + sizes1;
  const auto [indices, size] = sizes_to_indices(sizes);
  auto buffer = resize_buffer(buffer0, dim, size);
  auto target0 = indices;
  auto target1 = indices;
  add_to_second(target0, -sizes1);
  add_to_first(target1, sizes0);
  copy(buffer0, buffer, dim, ranges0, target0);
  copy(buffer1, buffer, dim, ranges1, target1);
  return variable::DataModel<bucket<T>>{std::move(indices), dim,
                                        std::move(buffer)};
}

template <class T>
auto concatenate_impl(const VariableConstView &var0,
                      const VariableConstView &var1) {
  return Variable{
      std::make_unique<variable::DataModel<bucket<T>>>(combine<T>(var0, var1))};
}

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

} // namespace buckets
} // namespace scipp::dataset
