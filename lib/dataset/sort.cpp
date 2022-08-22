// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/sort.h"
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
    std::vector<std::pair<T, scipp::index>> pairs;
    pairs.reserve(size);
    scipp::index i = 0;
    for (const auto &value : key.values<T>())
      pairs.emplace_back(value, i++);

    if (order == SortOrder::Ascending)
      std::sort(pairs.begin(), pairs.end(), [](const auto &a, const auto &b) {
        return nan_sensitive_less(a.first, b.first);
      });
    else
      std::sort(pairs.begin(), pairs.end(), [](const auto &a, const auto &b) {
        return nan_sensitive_less(b.first, a.first);
      });

    auto indices =
        makeVariable<scipp::index_pair>(Dims{key.dim()}, Shape{size});
    scipp::index j = 0;
    for (auto &index : indices.values<scipp::index_pair>().as_span()) {
      const auto start = pairs[j++].second;
      index = {start, start + 1};
    }
    return indices;
  }
};

Variable indices_for_sorting(const Variable &key, const SortOrder order) {
  return core::CallDType<
      double, float, int64_t, int32_t, bool, std::string,
      core::time_point>::apply<IndicesForSorting>(key.dtype(), key, order);
}

} // namespace

/// Return a Variable sorted based on key.
Variable sort(const Variable &var, const Variable &key, const SortOrder order) {
  return extract_ranges(indices_for_sorting(key, order), var, key.dim());
}

/// Return a DataArray sorted based on key.
DataArray sort(const DataArray &array, const Variable &key,
               const SortOrder order) {
  return extract_ranges(indices_for_sorting(key, order), array, key.dim());
}

/// Return a DataArray sorted based on coordinate.
DataArray sort(const DataArray &array, const Dim &key, const SortOrder order) {
  return sort(array, array.meta()[key], order);
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
