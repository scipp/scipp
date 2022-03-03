// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/sort.h"
#include "scipp/dataset/groupby.h"

namespace scipp::dataset {

/// Return a Variable sorted based on key.
Variable sort(const Variable &var, const Variable &key, const SortOrder order) {
  return sort(DataArray(var), key, order).data();
}

/// Return a DataArray sorted based on key.
DataArray sort(const DataArray &array, const Variable &key,
               const SortOrder order) {
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
