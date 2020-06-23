// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <map>

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

template <class T1, class T2> auto union_(const T1 &a, const T2 &b) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;

  for (const auto &[key, item] : a)
    out.emplace(key, item);

  for (const auto &item : b) {
    if (const auto it = a.find(item.first); it != a.end()) {
      core::expect::equals(item, *it);
    } else
      out.emplace(item.first, item.second);
  }
  return out;
}

/// Return intersection of maps, i.e., all items with matching names that
/// have matching content.
template <class Map> auto intersection(const Map &a, const Map &b) {
  std::map<std::string, Variable> out;
  for (const auto &[key, item] : a)
    if (const auto it = b.find(key); it != b.end() && it->second == item)
      out.emplace(key, item);
  return out;
}

/// Return a copy of map-like objects such as CoordView.
template <class T> auto copy_map(const T &map) {
  std::map<typename T::key_type, typename T::mapped_type> out;
  for (const auto &[key, item] : map)
    out.emplace(key, item);
  return out;
}

static inline void expectAlignedCoord(const Dim coord_dim,
                                      const VariableConstView &var,
                                      const Dim operation_dim) {
  // Coordinate is 2D, but the dimension associated with the coordinate is
  // different from that of the operation. Note we do not account for the
  // possibility that the coordinates actually align along the operation
  // dimension.
  if (var.dims().ndim() > 1)
    throw except::DimensionError(
        "VariableConstView Coord/Label has more than one dimension "
        "associated with " +
        to_string(coord_dim) +
        " and will not be reduced by the operation dimension " +
        to_string(operation_dim) + " Terminating operation.");
}

static constexpr auto no_realigned_support = []() {};
using no_realigned_support_t = decltype(no_realigned_support);

template <bool ApplyToData, class Func, class... Args>
DataArray apply_or_copy_dim_impl(const DataArrayConstView &a, Func func,
                                 const Dim dim, Args &&... args) {
  std::map<Dim, Variable> coords;
  // Note the `copy` call, ensuring that the return value of the ternary
  // operator can be moved. Without `copy`, the result of `func` is always
  // copied.
  for (auto &&[d, coord] : a.coords())
    if (coord.dims().ndim() == 0 || dim_of_coord(coord, d) != dim) {
      expectAlignedCoord(d, coord, dim);
      if constexpr (ApplyToData) {
        coords.emplace(d, coord.dims().contains(dim) ? func(coord, dim, args...)
                                                     : copy(coord));
      } else {
        coords.emplace(d, coord);
      }
    }

  std::map<std::string, Variable> attrs;
  for (auto &&[name, attr] : a.attrs())
    if (!attr.dims().contains(dim))
      attrs.emplace(name, attr);

  std::map<std::string, Variable> masks;
  for (auto &&[name, mask] : a.masks())
    if (!mask.dims().contains(dim))
      masks.emplace(name, mask);

  if constexpr (ApplyToData) {
    if (a.hasData()) {
      return DataArray(func(a.data(), dim, args...), std::move(coords),
                       std::move(masks), std::move(attrs), a.name());
    } else {
      if constexpr (std::is_base_of_v<no_realigned_support_t, Func>)
        throw std::logic_error("Operation cannot handle realigned data.");
      else
        return DataArray(func(a.dims(), a.unaligned(), dim, args...),
                         std::move(coords), std::move(masks), std::move(attrs),
                         a.name());
    }
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
/// on dim. The exception are multi-dimensional coords that depend on `dim`,
/// with two cases: (1) If the coord is a coord for `dim`, `func` is applied to
/// it, (2) if the coords is a coords for a dimension other than `dim`, a
/// CoordMismatchError is thrown.
template <class Func, class... Args>
DataArray apply_to_data_and_drop_dim(const DataArrayConstView &a, Func func,
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
DataArray apply_and_drop_dim(const DataArrayConstView &a, Func func,
                             const Dim dim, Args &&... args) {
  return apply_or_copy_dim_impl<false>(a, func, dim,
                                       std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_to_items(const DataArrayConstView &d, Func func,
                         Args &&... args) {
  return func(d, std::forward<Args>(args)...);
}

template <class... Args>
bool copy_attr(const VariableConstView &attr, const Dim dim, const Args &...) {
  return !attr.dims().contains(dim);
}
template <class... Args>
bool copy_attr(const VariableConstView &, const Args &...) {
  return true;
}

template <class Func, class... Args>
Dataset apply_to_items(const DatasetConstView &d, Func func, Args &&... args) {
  Dataset result;
  for (const auto &data : d)
    result.setData(data.name(), func(data, std::forward<Args>(args)...));
  for (auto &&[name, attr] : d.attrs())
    if (copy_attr(attr, args...))
      result.setAttr(name, attr);
  return result;
}

// Helpers for reductions for DataArray and Dataset, which include masks.
[[nodiscard]] Variable mean(const VariableConstView &var, const Dim dim,
                            const MasksConstView &masks);
VariableView mean(const VariableConstView &var, const Dim dim,
                  const MasksConstView &masks, const VariableView &out);
[[nodiscard]] Variable flatten(const VariableConstView &var, const Dim dim,
                               const MasksConstView &masks);
[[nodiscard]] Variable sum(const VariableConstView &var, const Dim dim,
                           const MasksConstView &masks);
VariableView sum(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks, const VariableView &out);

} // namespace scipp::dataset
