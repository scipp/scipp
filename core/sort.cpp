// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/except.h"
#include "scipp/core/indexed_slice_view.h"
#include "scipp/core/sort.h"
#include "scipp/core/tag_util.h"

namespace scipp::core {

template <class T> struct MakePermutation {
  static auto apply(const VariableConstProxy &key) {
    if (key.dims().ndim() != 1)
      throw except::DimensionError("Sort key must be 1-dimensional");

    // Variances are ignored for sorting.
    const auto &values = key.values<T>();

    std::vector<scipp::index> permutation(values.size());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::sort(
        permutation.begin(), permutation.end(),
        [&](scipp::index i, scipp::index j) { return values[i] < values[j]; });
    return permutation;
  }
};

auto makePermutation(const VariableConstProxy &key) {
  return CallDType<double, float, int64_t, int32_t, bool,
                   std::string>::apply<MakePermutation>(key.dtype(), key);
}

Variable sort(const VariableConstProxy &var, const VariableConstProxy &key) {
  return copy(IndexedSliceView{var, key.dims().inner(), makePermutation(key)});
}

DataArray sort(const DataConstProxy &array, const VariableConstProxy &key) {
  return copy(
      IndexedSliceView{array, key.dims().inner(), makePermutation(key)});
}

DataArray sort(const DataConstProxy &array, const Dim &key) {
  return sort(array, array.coords()[key]);
}

DataArray sort(const DataConstProxy &array, const std::string &key) {
  return sort(array, array.labels()[key]);
}

Dataset sort(const DatasetConstProxy &dataset, const VariableConstProxy &key) {
  return copy(
      IndexedSliceView{dataset, key.dims().inner(), makePermutation(key)});
}

Dataset sort(const DatasetConstProxy &dataset, const Dim &key) {
  return sort(dataset, dataset.coords()[key]);
}

Dataset sort(const DatasetConstProxy &dataset, const std::string &key) {
  return sort(dataset, dataset.labels()[key]);
}

} // namespace scipp::core
