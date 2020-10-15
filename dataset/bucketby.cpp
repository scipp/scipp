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

template <class A, class B>
auto find_sorting_permutation(const A &a, const B &b) {
  element_array<scipp::index> p(a.size(), core::default_init_elements);
  std::iota(p.begin(), p.end(), 0);
  core::parallel::parallel_sort(p.begin(), p.end(), [&](auto i, auto j) {
    return a[i] != a[j] ? a[i] < a[j] : b[i] < b[j];
  });
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

template <class T> struct MakePermutation1 {
  static auto apply(const VariableConstView &key) {
    expectValidGroupbyKey(key);
    // Using span over data since random access via ElementArrayView is slow.
    return makeVariable<scipp::index>(
        key.dims(), Values(find_sorting_permutation(scipp::span(
                        key.values<T>().data(), key.dims().volume()))));
  }
};

template <class A> struct MakePermutation2 {
  template <class B> struct Inner {
    static auto apply(const VariableConstView &a, const VariableConstView &b) {
      expectValidGroupbyKey(a);
      expectValidGroupbyKey(b);
      // Using span over data since random access via ElementArrayView is slow.
      return makeVariable<scipp::index>(
          a.dims(), Values(find_sorting_permutation(
                        scipp::span(a.values<A>().data(), a.dims().volume()),
                        scipp::span(b.values<B>().data(), b.dims().volume()))));
    }
  };
  static auto apply(const VariableConstView &a, const VariableConstView &b) {
    return core::CallDType<double, float, int64_t, int32_t, bool,
                           std::string>::apply<Inner>(b.dtype(), a, b);
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
  auto indices =
      core::CallDType<double, float, int64_t, int32_t, bool,
                      std::string>::apply<BoundIndices>(x.dtype(), x, edges);
  const auto &dims = indices.dims();
  const auto dim = dims.inner();
  const auto nbin = dims[dim] - 1;
  return zip(indices.slice({dim, 0, nbin}), indices.slice({dim, 1, nbin + 1}));
}

template <class T>
auto edge_indices(const VariableConstView &a, const VariableConstView &edges_a,
                  const T &b, const T &edges_b) {
  const auto outer_indices = edge_indices(a, edges_a);
  const auto outer_indices_ =
      outer_indices.values<std::pair<scipp::index, scipp::index>>();
  const auto size_b = scipp::size(edges_b);
  element_array<scipp::index> indices(outer_indices_.size() * size_b,
                                      core::default_init_elements);
  auto it = indices.begin();
  for (const auto &[begin, end] : outer_indices_) {
    auto inner_indices = edge_indices(b.subspan(begin, end - begin), edges_b);
    std::copy(inner_indices.begin(), inner_indices.end(), it);
    it += size_b;
  }
  return indices;
}

template <class T> struct BoundIndices2 {
  static auto apply(const VariableConstView &a,
                    const VariableConstView &edges_a,
                    const VariableConstView &b,
                    const VariableConstView &edges_b) {
    // Using span over data since random access via ElementArrayView is slow.
    const auto b_ = scipp::span(b.values<T>().data(), b.dims().volume());
    const auto edges_b_ =
        scipp::span(edges_b.values<T>().data(), edges_b.dims().volume());
    auto dims = merge(edges_a.dims(), edges_b.dims());
    const auto outer_dim = edges_a.dims().inner();
    dims.resize(outer_dim, dims[outer_dim] - 1);
    return makeVariable<scipp::index>(
        dims, Values(edge_indices(a, edges_a, b_, edges_b_)));
  }
};

auto edge_indices(const VariableConstView &a, const VariableConstView &edges_a,
                  const VariableConstView &b,
                  const VariableConstView &edges_b) {
  auto indices =
      core::CallDType<double, float, int64_t, int32_t, bool,
                      std::string>::apply<BoundIndices2>(b.dtype(), a, edges_a,
                                                         b, edges_b);
  const auto &dims = indices.dims();
  const auto dim = dims.inner();
  const auto nbin = dims[dim] - 1;
  return zip(indices.slice({dim, 0, nbin}), indices.slice({dim, 1, nbin + 1}));
}

template <class T>
auto call_sortby(const T &array, const VariableConstView &key) {
  return permute(
      array, key.dims().inner(),
      core::CallDType<double, float, int64_t, int32_t, bool,
                      std::string>::apply<MakePermutation1>(key.dtype(), key));
}

template <class T>
auto call_sortby(const T &array, const VariableConstView &a,
                 const VariableConstView &b) {
  return permute(
      array, a.dims().inner(),
      core::CallDType<double, float, int64_t, int32_t, bool,
                      std::string>::apply<MakePermutation2>(a.dtype(), a, b));
}

template <class... T>
DataArray sortby_impl(const DataArrayConstView &array, const T &... dims) {
  return call_sortby(array, array.coords()[dims]...);
}

DataArray sortby(const DataArrayConstView &array, const Dim dim) {
  return sortby_impl(array, dim);
}

DataArray sortby(const DataArrayConstView &array, const Dim dim0,
                 const Dim dim1) {
  return sortby_impl(array, dim0, dim1);
}

Variable bucketby(const DataArrayConstView &array, const Dim dim,
                  const VariableConstView &bins) {
  auto sorted = sortby(array, dim);
  const auto &key = sorted.coords()[dim];
  // Note that data outside bin bounds is *not* dropped. Should it?
  return buckets::from_constituents(edge_indices(key, bins), key.dims().inner(),
                                    std::move(sorted));
}

Variable bucketby(const DataArrayConstView &array, const Dim dim0,
                  const VariableConstView &bins0, const Dim dim1,
                  const VariableConstView &bins1) {
  auto sorted = sortby(array, dim0, dim1);
  const auto &key0 = sorted.coords()[dim0];
  const auto &key1 = sorted.coords()[dim1];
  return buckets::from_constituents(edge_indices(key0, bins0, key1, bins1),
                                    key0.dims().inner(), std::move(sorted));
}

} // namespace scipp::dataset
