// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/sort.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/extract.h"

namespace scipp::dataset {

namespace {

constexpr auto nan_sensitive_less = [](const auto &a, const auto &b) {
  if constexpr (std::is_floating_point_v<std::decay_t<decltype(a)>>)
    if (std::isnan(b))
      return !std::isnan(a);
  return a < b;
};

template <class T> struct IndicesForSorting {
  static Variable apply(const Variable &key, const SortOrder order) {
    const auto size = key.dims()[key.dim()];
    const auto values = key.values<T>();
    std::vector<std::pair<T, scipp::index>> key_index;
    key_index.reserve(size);

    {
      scipp::index i = 0;
      for (const auto &value : key.values<T>())
        key_index.emplace_back(value, i++);
    }

    if (order == SortOrder::Ascending)
      core::parallel::parallel_sort(
          key_index.begin(), key_index.end(), [](const auto &a, const auto &b) {
            return nan_sensitive_less(a.first, b.first);
          });
    else
      core::parallel::parallel_sort(
          key_index.begin(), key_index.end(), [](const auto &a, const auto &b) {
            return nan_sensitive_less(b.first, a.first);
          });

    auto indices =
        makeVariable<scipp::index_pair>(Dims{key.dim()}, Shape{size});
    std::transform(key_index.begin(), key_index.end(),
                   indices.values<scipp::index_pair>().as_span().begin(),
                   [](const auto &item) {
                     return std::pair{item.second, item.second + 1};
                   });
    return indices;
  }
};

Variable indices_for_sorting(const Variable &key, const SortOrder order) {
  return core::CallDType<
      double, float, int64_t, int32_t, bool, std::string,
      core::time_point>::apply<IndicesForSorting>(key.dtype(), key, order);
}

void require_same_shape(const Dimensions &var_dims, const Dimensions &key_dims,
                        const Dim dim) {
  if (var_dims[dim] != key_dims[dim])
    throw except::DimensionError(
        "Cannot sort: key for dimension " + to_string(dim) + " has length " +
        std::to_string(key_dims[dim]) + " while variable has length " +
        std::to_string(var_dims[dim]) + ". Lengths must agree.");
}

} // namespace

/// Return a Variable sorted based on key.
Variable sort(const Variable &var, const Variable &key, const SortOrder order) {
  require_same_shape(var.dims(), key.dims(), key.dim());
  return extract_ranges(indices_for_sorting(key, order), var, key.dim());
}

/// Return a DataArray sorted based on key.
DataArray sort(const DataArray &array, const Variable &key,
               const SortOrder order) {
  require_same_shape(array.dims(), key.dims(), key.dim());
  return extract_ranges(indices_for_sorting(key, order), array, key.dim());
}

/// Return a DataArray sorted based on coordinate.
DataArray sort(const DataArray &array, const Dim &key, const SortOrder order) {
  return sort(array, array.coords()[key], order);
}

/// Return a Dataset sorted based on key.
Dataset sort(const Dataset &dataset, const Variable &key,
             const SortOrder order) {
  return extract_ranges(indices_for_sorting(key, order), dataset, key.dim());
}

/// Return a Dataset sorted based on coordinate.
Dataset sort(const Dataset &dataset, const Dim &key, const SortOrder order) {
  return sort(dataset, dataset.coords()[key], order);
}

} // namespace scipp::dataset
