// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/dict.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"

namespace scipp::dataset {
template <bool ApplyToData, class Func, class... Args>
DataArray apply_or_copy_dim_impl(const DataArray &da, Func func, const Dim dim,
                                 Args &&...args) {
  auto data_result = func(
      [&]() {
        if constexpr (ApplyToData) {
          return da.data();
        } else {
          return da;
        }
      }(),
      dim, std::forward<Args>(args)...);
  const Sizes out_sizes = data_result.dims();

  const auto copy_independent = [&](const auto &mapping, const bool share) {
    std::decay_t<decltype(mapping)> out(out_sizes, {});
    for (auto &&[d, var] : mapping)
      if (!var.dims().contains(dim))
        out.set(d, share ? var : copy(var));
    return out;
  };
  auto coords = copy_independent(da.coords(), true);
  auto attrs = copy_independent(da.attrs(), true);
  auto masks = copy_independent(da.masks(), false);

  return DataArray(std::move(data_result), std::move(coords), std::move(masks),
                   std::move(attrs), da.name());
}

/// Helper for creating operations that return an object with modified data with
/// a dropped dimension or different dimension extent.
///
/// Examples are mostly reduction operations such as `sum` (dropping a
/// dimension), or `resize` (altering a dimension extent). Creates new data
/// array by applying `func` to data and dropping coords/masks/attrs depending
/// on dim.
template <class Func, class... Args>
DataArray apply_to_data_and_drop_dim(const DataArray &a, Func func,
                                     const Dim dim, Args &&...args) {
  return apply_or_copy_dim_impl<true>(a, func, dim,
                                      std::forward<Args>(args)...);
}

/// Helper for creating operations that return an object with a dropped
/// dimension or different dimension extent.
///
/// In contrast to `apply_to_data_and_drop_dim`, `func` is applied to the input
/// array, not just its data. This is useful for more complex operations such as
/// `histogram`, which require access to coords when computing output data.
template <class Func, class... Args>
DataArray apply_and_drop_dim(const DataArray &a, Func func, const Dim dim,
                             Args &&...args) {
  return apply_or_copy_dim_impl<false>(a, func, dim,
                                       std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_to_items(const DataArray &d, Func func, Args &&...args) {
  return func(d, std::forward<Args>(args)...);
}

template <class... Args>
bool copy_attr(const Variable &attr, const Dim dim, const Args &...) {
  return !attr.dims().contains(dim);
}
template <class... Args> bool copy_attr(const Variable &, const Args &...) {
  return true;
}

template <class Func, class... Args>
Dataset apply_to_items(const Dataset &d, Func func, Args &&...args) {
  Dataset result;
  for (const auto &data : d)
    result.setData(data.name(), func(data, std::forward<Args>(args)...));
  return result;
}

/// Return a copy of map-like objects such as Coords with `func` applied to each
/// item.
///
/// If `func` returns an invalid object it will not be inserted into the output
/// map. This can be used to drop/filter items.
template <class OutMapping, class Mapping, class Func>
auto transform_map(const Mapping &map, Func func) {
  OutMapping out;
  for (const auto &[key, item] : map) {
    auto transformed = func(item);
    if (transformed.is_valid()) {
      if constexpr (std::is_same_v<std::decay_t<Mapping>, Coords>)
        out.insert_or_assign(
            key, typename Mapping::aligned_mapped_type{std::move(transformed),
                                                       map.is_aligned(key)});
      else
        out.insert_or_assign(key, std::move(transformed));
    }
  }
  return out;
}

template <class T, class Func> DataArray transform(const T &a, Func func) {
  return DataArray(
      func(a.data()),
      transform_map<typename Coords::holder_type>(a.coords(), func),
      transform_map<typename Masks::holder_type>(a.masks(), func),
      transform_map<typename Attrs::holder_type>(a.attrs(), func), a.name());
}

[[nodiscard]] DataArray strip_if_broadcast_along(const DataArray &a,
                                                 const Dim dim);
[[nodiscard]] Dataset strip_if_broadcast_along(const Dataset &d, const Dim dim);

// Helpers for reductions for DataArray and Dataset, which include masks.
[[nodiscard]] Variable mean(const Variable &var, const Dim dim,
                            const Masks &masks);
[[nodiscard]] Variable nanmean(const Variable &var, const Dim dim,
                               const Masks &masks);
[[nodiscard]] Variable sum(const Variable &var, const Dim dim,
                           const Masks &masks);
[[nodiscard]] Variable nansum(const Variable &var, const Masks &masks);
[[nodiscard]] Variable nansum(const Variable &var, const Dim dim,
                              const Masks &masks);
[[nodiscard]] Variable max(const Variable &var, const Dim dim,
                           const Masks &masks);
[[nodiscard]] Variable nanmax(const Variable &var, const Dim dim,
                              const Masks &masks);
[[nodiscard]] Variable min(const Variable &var, const Dim dim,
                           const Masks &masks);
[[nodiscard]] Variable nanmin(const Variable &var, const Dim dim,
                              const Masks &masks);
[[nodiscard]] Variable all(const Variable &var, const Dim dim,
                           const Masks &masks);
[[nodiscard]] Variable any(const Variable &var, const Dim dim,
                           const Masks &masks);

[[nodiscard]] Variable
masked_data(const DataArray &array, const Dim dim,
            const std::optional<Variable> &fill_value = std::nullopt);

} // namespace scipp::dataset
