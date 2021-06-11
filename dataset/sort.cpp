// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/sort.h"
#include "scipp/dataset/groupby.h"

namespace scipp::dataset {
namespace {
template <class T> void ensure_distinct_buffers(const T &to_sort, const Variable &key) {
  if constexpr (std::is_same_v<std::decay_t<T>, DataArray>) {
    if (&to_sort.data().data() == &key.data()) {
      throw std::invalid_argument("The groupby key is equal to the data. "
                                  "Consider copying the key variable first.");
    }
  } else {
    for (auto it = to_sort.items_begin(); it != to_sort.items_end(); ++it) {
      const auto &[name, item] = *it;
      if (&item.data().data() == &key.data()) {
        throw std::invalid_argument(
            "The groupby key is equal to dataset entry '" + name +
            "'. Consider copying the key variable first.");
      }
    }
  }
}
} // namespace

/// Return a Variable sorted based on key.
Variable sort(const Variable &var, const Variable &key, const SortOrder order) {
  return sort(DataArray(var), key, order).data();
}

/// Return a DataArray sorted based on key.
DataArray sort(const DataArray &array, const Variable &key,
               const SortOrder order) {
  ensure_distinct_buffers(array, key);
  auto helper = array;
  const Dim dummy = Dim::InternalSort;
  helper.coords().set(dummy, key);
  helper = groupby(helper, dummy).copy(order);
  helper.coords().erase(dummy);
  return helper;
}

/// Return a DataArray sorted based on coordinate.
DataArray sort(const DataArray &array, const Dim &key, const SortOrder order) {
  return groupby(array, key).copy(order);
}

/// Return a Dataset sorted based on key.
Dataset sort(const Dataset &dataset, const Variable &key,
             const SortOrder order) {
  ensure_distinct_buffers(dataset, key);
  auto helper = dataset;
  const Dim dummy = Dim::InternalSort;
  helper.coords().set(dummy, key);
  helper = groupby(helper, dummy).copy(order);
  helper.coords().erase(dummy);
  return helper;
}

/// Return a Dataset sorted based on coordinate.
Dataset sort(const Dataset &dataset, const Dim &key, const SortOrder order) {
  return groupby(dataset, key).copy(order);
}

} // namespace scipp::dataset
