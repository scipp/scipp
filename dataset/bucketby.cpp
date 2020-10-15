// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/element/permute.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

#include "scipp/dataset/bucket.h"
#include "scipp/dataset/bucketby.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {


namespace {
static void expectValidGroupbyKey(const VariableConstView &key) {
  if (key.dims().ndim() != 1)
    throw except::DimensionError("Group-by key must be 1-dimensional");
  if (key.hasVariances())
    throw except::VariancesError("Group-by key cannot have variances");
}

template <class T> auto edge_indices(const T &x, const T &edges) {
  const auto size = scipp::size(edges);
  element_array<scipp::index> indices(size, core::default_init_elements);
  scipp::index current = 0;
  for (scipp::index i = 0; i < scipp::size(x); ++i) {
    while (current < size && x[i] >= edges[current])
      indices.data()[current++] = i;
  }
  for (; current < size; ++current)
    indices.data()[current] = scipp::size(x);
  return indices;
}

template <class T> auto find_sorting_permutation(const T &key) {
  element_array<scipp::index> p(key.size(), core::default_init_elements);
  std::iota(p.begin(), p.end(), 0);
  core::parallel::parallel_sort(
      p.begin(), p.end(), [&](auto i, auto j) { return key[i] < key[j]; });
  return p;
}

Variable permute(const VariableConstView &var, const Dim dim,
                 const VariableConstView &permutation) {
  return variable::transform(subspan_view(var, dim), permutation,
                             core::element::permute);
}

template <class T> struct BoundIndices {
  static auto apply(const VariableConstView &x,
                    const VariableConstView &edges) {
    // Using span over data since random access via ElementArrayView is slow.
    const auto x_ = scipp::span(x.values<T>().data(), x.dims().volume());
    const auto edges_ =
        scipp::span(edges.values<T>().data(), edges.dims().volume());
    return makeVariable<scipp::index>(edges.dims(),
                                      Values(edge_indices(x_, edges_)));
  }
};

template <class T> struct MakePermutation {
  static auto apply(const VariableConstView &key) {
    expectValidGroupbyKey(key);
    // Using span over data since random access via ElementArrayView is slow.
    const auto range = scipp::span(key.values<T>().data(), key.dims().volume());
    return makeVariable<scipp::index>(key.dims(),
                                      Values(find_sorting_permutation(range)));
  }
};

auto permute(const DataArrayConstView &data, const Dim dim,
             const VariableConstView &permutation) {
  return dataset::transform(data, [dim, permutation](const auto &var) {
    return var.dims().contains(dim) ? permute(var, dim, permutation)
                                    : copy(var);
  });
}
} // namespace


auto edge_indices(const VariableConstView &x, const VariableConstView &edges) {
  return core::CallDType<double, float, int64_t, int32_t, bool,
                         std::string>::apply<BoundIndices>(x.dtype(), x, edges);
}

template <class T>
auto call_bucketby(const T &array, const VariableConstView &key) {
  return permute(
      array, key.dims().inner(),
      core::CallDType<double, float, int64_t, int32_t, bool,
                      std::string>::apply<MakePermutation>(key.dtype(), key));
}


DataArray sortby(const DataArrayConstView &array, const Dim dim) {
  const auto &key = array.coords()[dim];
  return call_bucketby(array, key);
}

Variable bucketby(const DataArrayConstView &array, const Dim dim,
                  const VariableConstView &bins) {
  auto sorted = sortby(array, dim);
  const auto &key = sorted.coords()[dim];
  auto indices = edge_indices(key, bins);
  const auto &dims = bins.dims();
  const auto bin_dim = dims.inner();
  const auto nbin = dims[bin_dim] - 1;
  // Note that data outside bin bounds is *not* dropped. Should it?
  return buckets::from_constituents(zip(indices.slice({bin_dim, 0, nbin}),
                                        indices.slice({bin_dim, 1, nbin + 1})),
                                    key.dims().inner(), std::move(sorted));
}

} // namespace scipp::dataset
