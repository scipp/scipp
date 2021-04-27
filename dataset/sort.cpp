// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/sort.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {
Dim nonclashing_name(const Coords &coords) {
  std::string name("dummy");
  for (const auto &item : coords)
    name += item.first.name();
  return Dim(name);
}
} // namespace

/// Return a Variable sorted based on key.
Variable sort(const Variable &var, const Variable &key, const SortOrder order) {
  return sort(DataArray(var), key, order).data();
}

/// Return a DataArray sorted based on key.
DataArray sort(const DataArray &array, const Variable &key,
               const SortOrder order) {
  auto helper = array;
  const auto dummy = nonclashing_name(helper.coords());
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
  const auto dummy = nonclashing_name(helper.coords());
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
