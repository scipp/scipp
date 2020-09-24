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
#include "scipp/dataset/shape.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"
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

Variable concatenate(const VariableConstView &var0,
                     const VariableConstView &var1) {
  const auto &[ranges0, dim0, buffer0] = var0.constituents<bucket<DataArray>>();
  const auto &[ranges1, dim1, buffer1] = var1.constituents<bucket<DataArray>>();
  core::expect::equals(ranges0.dims(), ranges1.dims());
  // core::expect::equals(dim0, dim1);
  const Dim dim = dim0;
  // const auto dims = concatenate(buffer0.dims(), buffer1.dims(), dim);
  // TODO This copies, but is later overwritten, should be avoided
  auto buffer = dataset::concatenate(buffer0, buffer1, dim);
  const auto sizes0 = sizes(ranges0);
  const auto sizes1 = sizes(ranges1);
  const auto [indices, size] = sizes_to_indices(sizes0 + sizes1);
  auto target0 = indices;
  auto target1 = indices;
  add_to_second(target0, -sizes1);
  add_to_first(target1, sizes0);
  copy(buffer0, buffer, dim, ranges0, target0);
  copy(buffer1, buffer, dim, ranges1, target1);
  return Variable{std::make_unique<variable::DataModel<bucket<DataArray>>>(
      indices, dim, std::move(buffer))};
}

} // namespace buckets
} // namespace scipp::dataset
