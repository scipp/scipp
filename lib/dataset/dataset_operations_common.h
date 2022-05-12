// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <unordered_map>

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/arithmetic.h"

namespace scipp::dataset {

template <class T1, class T2>
auto union_(const T1 &a, const T2 &b, const std::string_view opname) {
  std::unordered_map<typename T1::key_type, typename T1::mapped_type> out;

  for (const auto &[key, item] : a)
    out.emplace(key, item);

  for (const auto &item : b) {
    if (const auto it = a.find(item.first); it != a.end()) {
      expect::matching_coord(it->first, it->second, item.second, opname);
    } else
      out.emplace(item.first, item.second);
  }
  return out;
}

/// Return intersection of maps, i.e., all items with matching names that
/// have matching content.
template <class Map> auto intersection(const Map &a, const Map &b) {
  std::unordered_map<typename Map::key_type, Variable> out;
  for (const auto &[key, item] : a)
    if (const auto it = b.find(key);
        it != b.end() && equals_nan(it->second, item))
      out.emplace(key, item);
  return out;
}

/// Return a copy of map-like objects such as CoordView.
template <class T> auto copy_map(const T &map) {
  std::unordered_map<typename T::key_type, typename T::mapped_type> out;
  for (const auto &[key, item] : map)
    out.emplace(key, copy(item));
  return out;
}

template <bool ApplyToData, class Func, class... Args>
DataArray apply_or_copy_dim_impl(const DataArray &a, Func func, const Dim dim,
                                 Args &&... args) {
  const auto copy_independent = [&](auto &coords_, const auto &view,
                                    const bool share) {
    for (auto &&[d, coord] : view)
      if (!coord.dims().contains(dim))
        coords_.emplace(d, share ? coord : copy(coord));
  };
  std::unordered_map<Dim, Variable> coords;
  copy_independent(coords, a.coords(), true);

  std::unordered_map<Dim, Variable> attrs;
  copy_independent(attrs, a.attrs(), true);

  std::unordered_map<std::string, Variable> masks;
  copy_independent(masks, a.masks(), false);

  if constexpr (ApplyToData) {
    return DataArray(func(a.data(), dim, args...), std::move(coords),
                     std::move(masks), std::move(attrs), a.name());
  } else {
    return DataArray(func(a, dim, std::forward<Args>(args)...),
                     std::move(coords), std::move(masks), std::move(attrs),
                     a.name());
  }
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
                                     const Dim dim, Args &&... args) {
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
                             Args &&... args) {
  return apply_or_copy_dim_impl<false>(a, func, dim,
                                       std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_to_items(const DataArray &d, Func func, Args &&... args) {
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
Dataset apply_to_items(const Dataset &d, Func func, Args &&... args) {
  Dataset result;
  for (const auto &data : d)
    result.setData(data.name(), func(data, std::forward<Args>(args)...));
  return result;
}

/// Return a copy of map-like objects such as Coords with `func` applied to each
/// item.
template <class T, class Func> auto transform_map(const T &map, Func func) {
  std::unordered_map<typename T::key_type, typename T::mapped_type> out;
  for (const auto &[key, item] : map)
    out.emplace(key, func(item));
  return out;
}

template <class T, class Func> DataArray transform(const T &a, Func func) {
  return DataArray(func(a.data()), transform_map(a.coords(), func),
                   transform_map(a.masks(), func),
                   transform_map(a.attrs(), func), a.name());
}

[[nodiscard]] DataArray strip_if_broadcast_along(const DataArray &a,
                                                 const Dim dim);
[[nodiscard]] Dataset strip_if_broadcast_along(const Dataset &d, const Dim dim);

[[nodiscard]] DataArray strip_edges_along(const DataArray &da, const Dim dim);
[[nodiscard]] Dataset strip_edges_along(const Dataset &ds, const Dim dim);

// Helpers for reductions for DataArray and Dataset, which include masks.
[[nodiscard]] Variable mean(const Variable &var, const Dim dim,
                            const Masks &masks);
[[nodiscard]] Variable nanmean(const Variable &var, const Dim dim,
                               const Masks &masks);
[[nodiscard]] Variable sum(const Variable &var, const Dim dim,
                           const Masks &masks);
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

[[nodiscard]] Variable masked_data(const DataArray &array, const Dim dim);

} // namespace scipp::dataset
