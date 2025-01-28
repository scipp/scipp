// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"

#include "scipp/variable/creation.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

auto union_(const Dataset &a, const Dataset &b) {
  std::map<std::string, DataArray> out;

  for (const auto &item : a)
    out.emplace(item.name(), item);
  for (const auto &item : b) {
    if (const auto it = a.find(item.name()); it != a.end())
      core::expect::equals(item, *it);
    else
      out.emplace(item.name(), item);
  }
  return out;
}

Dataset merge(const Dataset &a, const Dataset &b) {
  return Dataset(union_(a, b), union_(a.coords(), b.coords(), "merge"));
}

/// Return a copy of dict-like objects as a core::Dict.
template <class Mapping> auto copy_map(const Mapping &map) {
  core::Dict<typename Mapping::key_type, typename Mapping::mapped_type> out;
  for (const auto &[key, item] : map)
    out.insert_or_assign(key, copy(item));
  return out;
}

Coords copy(const Coords &coords) { return {coords.sizes(), copy_map(coords)}; }
Masks copy(const Masks &masks) { return {masks.sizes(), copy_map(masks)}; }

/// Return a deep copy of a DataArray.
DataArray copy(const DataArray &array) {
  // When data is copied we generally need to copy masks, since masks are
  // typically modified when data is modified.
  return DataArray(copy(array.data()), copy(array.coords()),
                   copy(array.masks()), array.name());
}

/// Return a deep copy of a Dataset.
Dataset copy(const Dataset &dataset) {
  Dataset out{{}, copy(dataset.coords())};
  for (const auto &item : dataset) {
    out.setData(item.name(), copy(item));
  }
  return out;
}

namespace {
template <class T> void copy_item(const DataArray &from, T &&to) {
  for (const auto &[name, mask] : from.masks())
    copy(mask, to.masks()[name]);
  copy(from.data(), to.data());
}
} // namespace

/// Copy data array to output data array
DataArray &copy(const DataArray &array, DataArray &out) {
  for (const auto &[dim, coord] : array.coords())
    copy(coord, out.coords()[dim]);
  copy_item(array, out);
  return out;
}

/// Copy data array to output data array
DataArray copy(const DataArray &array, DataArray &&out) {
  copy(array, out);
  return std::move(out);
}

/// Copy dataset to output dataset
Dataset &copy(const Dataset &dataset, Dataset &out) {
  for (const auto &[dim, coord] : dataset.coords())
    copy(coord, out.coords()[dim]);
  for (const auto &array : dataset)
    copy_item(array, out[array.name()]);
  return out;
}

/// Copy dataset to output dataset
Dataset copy(const Dataset &dataset, Dataset &&out) {
  copy(dataset, out);
  return std::move(out);
}

/// Return data of data array, applying masks along dim if applicable.
///
/// Only in the latter case a copy is returned. Masked values are replaced by
/// fill_value. If not provided, values are replaced by zero.
Variable masked_data(const DataArray &array, const Dim dim,
                     const std::optional<Variable> &fill_value) {
  const auto mask = irreducible_mask(array.masks(), dim);
  if (mask.is_valid()) {
    const auto &data = array.data();
    const auto fill = fill_value.value_or(zero_like(array.data()));
    return where(mask, fill, data);
  } else
    return array.data();
}

namespace {
template <class Dict> auto strip_(const Dict &dict, const Dim dim) {
  Dict stripped(dict.sizes(), {});
  for (const auto &[key, value] : dict)
    if (value.dims().contains(dim))
      stripped.set(key, value);
  return stripped;
}
} // namespace

DataArray strip_if_broadcast_along(const DataArray &a, const Dim dim) {
  return {a.data(), strip_(a.coords(), dim), strip_(a.masks(), dim), a.name()};
}

Dataset strip_if_broadcast_along(const Dataset &d, const Dim dim) {
  Dataset stripped;
  stripped.setCoords(strip_(d.coords(), dim));
  for (auto &&item : d)
    if (item.dims().contains(dim))
      stripped.setData(item.name(), strip_if_broadcast_along(item, dim));
  return stripped;
}

} // namespace scipp::dataset
