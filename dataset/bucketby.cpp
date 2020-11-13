// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/element/histogram.h"
#include "scipp/core/element/permute.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/bucketby.h"
#include "scipp/dataset/except.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {

template <class T> auto find_sorting_permutation(const T &key) {
  element_array<scipp::index> p(key.size(), core::default_init_elements);
  std::iota(p.begin(), p.end(), 0);
  core::parallel::parallel_sort(
      p.begin(), p.end(), [&](auto i, auto j) { return key[i] < key[j]; });
  return p;
}

Variable bin_index(const VariableConstView &var,
                   const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  if (all(is_linspace(edges, dim)).value<bool>()) {
    return variable::transform(var, subspan_view(edges, dim),
                               core::element::bin_index_linspace);
  } else {
    if (!is_sorted(edges, dim))
      throw except::BinEdgeError("Bin edges must be sorted.");
    return variable::transform(var, subspan_view(edges, dim),
                               core::element::bin_index_sorted_edges);
  }
}

Variable groups_to_map(const VariableConstView &var, const Dim dim) {
  return variable::transform(subspan_view(var, dim),
                             core::element::groups_to_map);
}

Variable group_index(const VariableConstView &var,
                     const VariableConstView &groups) {
  const auto dim = groups.dims().inner();
  const auto map = groups_to_map(groups, dim);
  return variable::transform(var, map, core::element::group_index);
}

auto shrink(const Dimensions &dims) {
  auto shrunk = dims;
  shrunk.resize(dims.inner(), dims[dims.inner()] - 1);
  return shrunk;
}

/// `dims` provides dimensions of pre-existing binning, `edges` define
/// additional dimensions.
Variable bin_sizes(const VariableConstView &indices, Dimensions dims) {
  Variable sizes = makeVariable<scipp::index>(dims);
  const auto s = sizes.values<scipp::index>().as_span();
  for (const auto i : indices.values<scipp::index>().as_span())
    if (i >= 0)
      ++s[i];
  return sizes;
}

template <class T> struct Bin {
  static auto apply(const VariableConstView &var,
                    const VariableConstView &indices,
                    const VariableConstView &sizes) {
    auto [begin, total_size] = sizes_to_begin(sizes);
    auto dims = var.dims();
    // Output may be smaller since values outside bins are dropped.
    dims.resize(dims.inner(), total_size);
    auto binned = variable::variableFactory().create(
        var.dtype(), dims, var.unit(), var.hasVariances());
    const auto indices_ = indices.values<scipp::index>().as_span();
    const auto values = var.values<T>().as_span();
    const auto binned_values = binned.values<T>().as_span();
    const auto current = begin.values<scipp::index>().as_span();
    const auto size = scipp::size(values);
    if (var.hasVariances()) {
      const auto variances = var.variances<T>().as_span();
      const auto binned_variances = binned.variances<T>().as_span();
      for (scipp::index i = 0; i < size; ++i) {
        const auto i_bin = indices_[i];
        if (i_bin < 0)
          continue;
        binned_variances[current[i_bin]] = variances[i];
        binned_values[current[i_bin]++] = values[i];
      }
    } else {
      for (scipp::index i = 0; i < size; ++i) {
        const auto i_bin = indices_[i];
        if (i_bin < 0)
          continue;
        binned_values[current[i_bin]++] = values[i];
      }
    }
    return binned;
  }
};

auto bin(const VariableConstView &var, const VariableConstView &indices,
         const VariableConstView &sizes) {
  return core::CallDType<double, float, int64_t, int32_t, bool, Eigen::Vector3d,
                         std::string>::apply<Bin>(var.dtype(), var, indices,
                                                  sizes);
}

auto bin(const DataArrayConstView &data, const VariableConstView &indices,
         const VariableConstView &sizes) {
  return dataset::transform(data, [indices, sizes](const auto &var) {
    return var.dims().contains(indices.dims().inner())
               ? bin(var, indices, sizes)
               : copy(var);
  });
}

Variable permute(const VariableConstView &var, const Dim dim,
                 const VariableConstView &permutation) {
  return variable::transform(subspan_view(var, dim), permutation,
                             core::element::permute);
}

template <class T> struct MakePermutation {
  static auto apply(const VariableConstView &key) {
    expect::isKey(key);
    // Using span over data since random access via ElementArrayView is slow.
    return makeVariable<scipp::index>(
        key.dims(),
        Values(find_sorting_permutation(key.values<T>().as_span())));
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

template <class T>
auto call_sortby(const T &array, const VariableConstView &key) {
  return permute(
      array, key.dims().inner(),
      core::CallDType<double, float, int64_t, int32_t, bool,
                      std::string>::apply<MakePermutation>(key.dtype(), key));
}

template <class... T>
DataArray sortby_impl(const DataArrayConstView &array, const T &... dims) {
  return call_sortby(array, array.coords()[dims]...);
}

DataArray sortby(const DataArrayConstView &array, const Dim dim) {
  return sortby_impl(array, dim);
}

namespace {
DataArray bucketby_impl(const DataArrayConstView &array,
                        const std::vector<VariableConstView> &edges,
                        Variable indices = Variable{},
                        Dim bucket_dim = Dim::Invalid,
                        Dimensions dims = Dimensions{}) {
  for (const auto &edge : edges) {
    const auto coord = array.coords()[edge.dims().inner()];
    // TODO This implicit way of distinguishing edges and non-edges is probably
    // not the ultimate desired solution.
    const bool is_edges =
        (edge.dtype() == dtype<float>) || (edge.dtype() == dtype<double>);
    const auto inner_indices =
        is_edges ? bin_index(coord, edge) : group_index(coord, edge);
    const auto nbin = edge.dims()[edge.dims().inner()] - (is_edges ? 1 : 0);
    dims = merge(dims, is_edges ? shrink(edge.dims()) : edge.dims());
    if (indices) {
      if (bucket_dim != coord.dims().inner())
        throw except::DimensionError(
            "Coords of data to be bucketed have inconsistent dimensions " +
            to_string(bucket_dim) + " and " + to_string(coord.dims().inner()) +
            ". Data that can be bucketed should generally resemble a table, "
            "with a coord column for each bucketed dimension.");
      indices = variable::transform<scipp::index>(
          indices, inner_indices,
          overloaded{[](const units::Unit &, const units::Unit &) {
                       return units::one;
                     },
                     [nbin](const auto i0, const auto i1) {
                       return i0 < 0 || i1 < 0 ? -1 : i0 * nbin + i1;
                     }});
    } else {
      indices = inner_indices;
    }
    bucket_dim = coord.dims().inner();
  }
  const auto sizes = bin_sizes(indices, dims);
  const auto [begin, total_size] = sizes_to_begin(sizes);
  const auto end = begin + sizes;
  // TODO We probably want to omit the coord used for grouping in the non-edge
  // case, since it just contains the same value duplicated for every row in the
  // bin.
  // Note that we should then also recreate that variable in concatenate, to
  // ensure that those operations are reversible.
  auto binned = bin(array, indices, sizes);
  std::map<Dim, Variable> coords;
  for (const auto &edge : edges)
    coords[edge.dims().inner()] = copy(edge);
  return {make_bins(zip(begin, end), bucket_dim, std::move(binned)),
          std::move(coords)};
}
} // namespace

DataArray bucketby(const DataArrayConstView &array,
                   const std::vector<VariableConstView> &edges) {
  DataArrayConstView maybe_concat(array);
  DataArray tmp;
  if (array.dtype() == dtype<bucket<DataArray>>) {
    // TODO The need for this check may be an indicator that we should support
    // adding another bucketed dimension via a separate function instead of
    // having this dual-purpose `bucketby`.
    for (const auto &edge : edges)
      if (array.dims().contains(edge.dims().inner())) {
        // TODO Very inefficient if new edges extract only a small fraction
        tmp = buckets::concatenate(maybe_concat, edge.dims().inner());
        maybe_concat = DataArrayView(tmp);
      }
    const auto &[begin_end, dim, buffer] =
        maybe_concat.data().constituents<bucket<DataArray>>();
    auto indices =
        makeVariable<scipp::index>(Dims{dim}, Shape{buffer.dims()[dim]});
    indices -= 1 * units::one;
    const auto indices_ = indices.values<scipp::index>().as_span();
    scipp::index current = 0;
    for (const auto [begin, end] : begin_end.values<index_pair>().as_span()) {
      for (scipp::index i = begin; i < end; ++i)
        indices_[i] = current;
      ++current;
    }
    auto bucketed =
        bucketby_impl(buffer, edges, std::move(indices), dim, begin_end.dims());
    copy_metadata(maybe_concat, bucketed);
    return bucketed;
  } else {
    return bucketby_impl(maybe_concat, edges);
  }
}

} // namespace scipp::dataset
