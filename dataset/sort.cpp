// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/element/sort.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/indexed_slice_view.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

#include "dataset_operations_common.h"

using scipp::variable::IndexedSliceView;

namespace scipp::dataset {

template <class T> struct MakePermutation {
  static auto apply(const VariableConstView &key) {
    if (key.dims().ndim() != 1)
      throw except::DimensionError("Sort key must be 1-dimensional");

    // Variances are ignored for sorting.
    const auto &values = key.values<T>();

    std::vector<scipp::index> permutation(values.size());
    std::iota(permutation.begin(), permutation.end(), 0);
    std::sort(
        permutation.begin(), permutation.end(),
        [&](scipp::index i, scipp::index j) { return values[i] < values[j]; });
    return makeVariable<scipp::index>(key.dims(),
                                      Values(std::move(permutation)));
  }
};

static auto makePermutation(const VariableConstView &key) {
  return core::CallDType<double, float, int64_t, int32_t, bool,
                         std::string>::apply<MakePermutation>(key.dtype(), key);
}

Variable permute(const VariableConstView &var,
                 const VariableConstView &permutation) {
  const Dim dim = permutation.dims().inner();
  if (var.dims().inner() == dim) {
    Variable permuted(var);
    variable::transform_in_place(variable::subspan_view(permuted, dim),
                                 variable::subspan_view(permutation, dim),
                                 core::element::permute_in_place);
    return permuted;
  } else {
    const auto perm = permutation.values<scipp::index>();
    std::vector<scipp::index> indices(perm.begin(), perm.end());
    return concatenate(IndexedSliceView{var, dim, indices});
  }
}

/// Return a Variable sorted based on key.
Variable sort(const VariableConstView &var, const VariableConstView &key) {
  return permute(var, makePermutation(key));
}

namespace {
template <class T>
auto permute(const T &var, const Dim dim, const VariableConstView &key) {
  if (dim != key.dims().inner())
    throw except::DimensionError("Sort key must depend on sort dimension");
  if constexpr (std::is_same_v<T, DataArrayConstView>)
    return dataset::permute(var.data(), key);
  else
    return dataset::permute(var, key);
}

Dimensions permute(const Dimensions &dims, const Dim,
                   const VariableConstView &) {
  return dims;
}
} // namespace

DataArray permute(const DataArrayConstView &array, const Dim dim,
                  const VariableConstView &permutation) {
  return apply_or_copy_dim(
      array, [](auto &&... _) { return permute(_...); }, dim, permutation);
}

/// Return a DataArray sorted based on key.
DataArray sort(const DataArrayConstView &array, const VariableConstView &key) {
  return permute(array, key.dims().inner(), makePermutation(key));
}

/// Return a DataArray sorted based on coordinate.
DataArray sort(const DataArrayConstView &array, const Dim &key) {
  return sort(array, array.coords()[key]);
}

void sort_v3(const VariableConstView &key, const VariableView &value) {
  const Dim dim = key.dims().inner();
  if (!value.dims().contains(dim))
    return;
  Variable key_(key);
  variable::transform_in_place(variable::subspan_view(key_, dim),
                               variable::subspan_view(value, dim),
                               core::element::sort);
}

/// Return a Dataset sorted based on key.
Dataset sort(const DatasetConstView &dataset, const VariableConstView &key) {
  Dataset sorted(dataset);
  std::vector<VariableView> to_sort;
  for (const auto &item : sorted)
    to_sort.emplace_back(item.data());
  for (const auto &item : sorted.coords())
    to_sort.emplace_back(item.second);
  for (const auto &item : sorted.masks())
    to_sort.emplace_back(item.second);
  for (const auto &item : sorted.attrs())
    to_sort.emplace_back(item.second);
  core::parallel::parallel_for(
      core::parallel::blocked_range(0, to_sort.size()), [&](const auto &range) {
        for (scipp::index i = range.begin(); i != range.end(); ++i)
          sort_v3(key, to_sort[i]);
      });

  return sorted;
  return apply_to_items(
      dataset, [](auto &&... _) { return permute(_...); }, key.dims().inner(),
      makePermutation(key));
}

/// Return a Dataset sorted based on coordinate.
Dataset sort(const DatasetConstView &dataset, const Dim &key) {
  return sort(dataset, dataset.coords()[key]);
}

} // namespace scipp::dataset
