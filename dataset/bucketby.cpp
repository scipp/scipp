// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <iostream>
#include <numeric>

#include "scipp/core/element/cumulative.h"
#include "scipp/core/element/histogram.h"
#include "scipp/core/element/permute.h"
#include "scipp/core/parallel.h"
#include "scipp/core/tag_util.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
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

void update_indices_by_binning(const VariableView &indices,
                               const VariableConstView &key,
                               const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  if (all(is_linspace(edges, dim)).value<bool>()) {
    variable::transform_in_place(
        indices, key, subspan_view(edges, dim),
        core::element::update_indices_by_binning_linspace);
  } else {
    if (!is_sorted(edges, dim))
      throw except::BinEdgeError("Bin edges must be sorted.");
    variable::transform_in_place(
        indices, key, subspan_view(edges, dim),
        core::element::update_indices_by_binning_sorted_edges);
  }
}

Variable groups_to_map(const VariableConstView &var, const Dim dim) {
  return variable::transform(subspan_view(var, dim),
                             core::element::groups_to_map);
}

void update_indices_by_grouping(const VariableView &indices,
                                const VariableConstView &key,
                                const VariableConstView &groups) {
  const auto dim = groups.dims().inner();
  const auto map = groups_to_map(groups, dim);
  variable::transform_in_place(indices, key, map,
                               core::element::update_indices_by_grouping);
}

template <class T> Variable as_subspan_view(T &&binned) {
  if (binned.dtype() == dtype<bin<Variable>>) {
    auto &&[indices, dim, buffer] =
        binned.template constituents<bin<Variable>>();
    return subspan_view(buffer, dim, indices);
  } else if (binned.dtype() == dtype<bin<VariableView>>) {
    auto &&[indices, dim, buffer] =
        binned.template constituents<bin<VariableView>>();
    return subspan_view(buffer, dim, indices);
  } else {
    auto &&[indices, dim, buffer] =
        binned.template constituents<bin<VariableConstView>>();
    return subspan_view(buffer, dim, indices);
  }
}

Variable front(const VariableConstView &var) {
  return variable::transform(as_subspan_view(var), core::element::front);
}

/// indices is a binned variable with sub-bin indices, i.e., new bins within
/// bins
Variable bin_sizes2(const VariableConstView &sub_bin, const scipp::index nbin) {
  const auto nbins = broadcast(nbin * units::one, sub_bin.dims());
  auto sizes = resize(sub_bin, nbins);
  buckets::reserve(sizes, nbins);
  variable::transform_in_place(
      as_subspan_view(sizes), as_subspan_view(sub_bin),
      core::element::count_indices); // transform bins, not bin element
  return sizes;
}

// Notes on threaded impl:
//
// If event is in existing bin, it will stay there. Can use existing bin sizes
// as view over output, and threading will work
// Example:
// - existing pixel binning
// - now bin TOF
// - output has (potentially smaller) pixel bins, use subspan_view
// transform_in_place(subspan_view(out, out_buffer_dim, out_bin_range), data,
// indices)
// => threading over pixel dim
//
// What about binning flat data? order is completely random?
// - sc.bins() with dummy indices => fake 1d binning
//   - maybe use make_non_owning_bins?
// - transpose bucket end/begin (essentially making new dim an outer dim)
//   - can we achieve this by specifying reverse dim order, with no extra code?
// - proceeed as above

template <class Out>
auto bin2(Out &&out, const Variable &in, const VariableConstView &sizes,
          const VariableConstView &indices) {
  transform_in_place(as_subspan_view(out), as_subspan_view(sizes),
                     as_subspan_view(in), as_subspan_view(indices),
                     core::element::bin);
}

void exclusive_scan_bins(const VariableView &var) {
  transform_in_place(as_subspan_view(var), core::element::exclusive_scan);
}

template <class T>
auto bin2(const VariableConstView &data, const VariableConstView &indices,
          const Dimensions &dims) {
  std::vector<Dim> rebinned_dims;
  for (const auto dim : dims.labels())
    if (data.dims().contains(dim))
      rebinned_dims.push_back(dim);

  const auto &[ignored_input_indices, buffer_dim, in_buffer] =
      data.constituents<bucket<T>>();
  const auto nbin = dims.volume();
  auto output_bin_sizes = bin_sizes2(indices, nbin);
  auto offsets = output_bin_sizes;
  std::cout << "output_bin_sizes\n" << output_bin_sizes << '\n';
  Variable filtered_input_bin_size;
  if (rebinned_dims.empty()) {
    filtered_input_bin_size = buckets::sum(output_bin_sizes);
    exclusive_scan_bins(offsets);
  } else {
    filtered_input_bin_size = front(output_bin_sizes);
    for (const auto dim : rebinned_dims) {
      output_bin_sizes = sum(output_bin_sizes, dim);
      // TODO I don't think we can stack the operation like this
      exclusive_scan(offsets, dim);
    }
    auto output_bin_offsets = output_bin_sizes;
    exclusive_scan_bins(output_bin_offsets);
    offsets += output_bin_offsets;
  }
  std::cout << "offsets\n" << offsets << '\n';
  std::cout << "output_bin_sizes\n" << output_bin_sizes << '\n';
  std::cout << "filtered_input_bin_size\n" << filtered_input_bin_size << '\n';
  auto [begin, total_size] = sizes_to_begin(filtered_input_bin_size);
  if (rebinned_dims.empty())
    offsets += begin;
  std::cout << "begin\n" << begin << '\n';
  total_size = sum(buckets::sum(output_bin_sizes)).value<scipp::index>();
  std::cout << "total_size " << total_size << '\n';
  auto out_buffer = resize_default_init(in_buffer, buffer_dim, total_size);
  // all point to global range, offsets handles the rst
  begin -= begin;
  const auto filtered_input_bin_ranges =
      zip(begin, begin + total_size * units::one);
  // const auto filtered_input_bin_ranges =
  //    zip(begin, begin + buckets::sum(output_bin_sizes));
  const auto as_bins = [&](const auto &var) {
    return make_non_owning_bins(filtered_input_bin_ranges, buffer_dim, var);
  };

  const auto input_bins = bins_view<T>(data);
  // const auto output_bins = bins_view<DataArrayView>(binned);
  bin2(as_bins(out_buffer.data()), input_bins.data(), offsets, indices);
  for (const auto &[dim, coord] : out_buffer.coords()) {
    if (coord.dims().contains(buffer_dim))
      bin2(as_bins(out_buffer.coords()[dim]), input_bins.coords()[dim], offsets,
           indices);
  }
  for (const auto &[dim, coord] : out_buffer.masks()) {
    if (coord.dims().contains(buffer_dim))
      bin2(as_bins(out_buffer.masks()[dim]), input_bins.masks()[dim], offsets,
           indices);
  }

  auto output_dims = data.dims();
  for (const auto dim : dims.labels())
    if (output_dims.contains(dim))
      output_dims.erase(dim);
  output_dims = merge(output_dims, dims);
  std::cout << "output_dims " << output_dims << '\n';
  std::cout << output_bin_sizes << '\n';
  // TODO Why does reshape not throw if volume is too small?
  const auto bin_sizes =
      reshape(std::get<2>(output_bin_sizes.constituents<bucket<Variable>>()),
              output_dims);
  std::cout << bin_sizes << '\n';
  // TODO take into account rebin
  std::tie(begin, total_size) = sizes_to_begin(bin_sizes);
  const auto end = begin + bin_sizes;
  std::cout << begin << '\n';
  std::cout << end << '\n';
  return make_bins(zip(begin, end), buffer_dim, std::move(out_buffer));
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
template <class T>
DataArray bucketby_impl(const VariableConstView &var,
                        const std::vector<VariableConstView> &edges,
                        const std::vector<VariableConstView> &groups,
                        const std::vector<Dim> &) {
  const auto &[begin_end, dim, array] = var.constituents<bucket<T>>();
  const auto input_bins = bins_view<T>(var);
  const auto [begin, end] = unzip(begin_end);
  const auto input_bin_sizes = bucket_sizes(array.data());
  auto indices =
      make_bins(copy(begin_end), dim, makeVariable<scipp::index>(array.dims()));
  Dimensions dims;
  for (const auto &group : groups) {
    const auto group_dim = group.dims().inner();
    const auto coord = input_bins.coords()[group_dim];
    update_indices_by_grouping(indices, coord, group);
    dims.addInner(group_dim, group.dims()[group_dim]);
  }
  for (const auto &edge : edges) {
    const auto edge_dim = edge.dims().inner();
    const auto coord = input_bins.coords()[edge_dim];
    // TODO does this mean we can do ragged only in two steps?
    // i.e., indices must have same outer dim as edge
    update_indices_by_binning(indices, coord, edge);
    dims.addInner(edge_dim, edge.dims()[edge_dim] - 1);
  }
  // TODO We probably want to omit the coord used for grouping in the non-edge
  // case, since it just contains the same value duplicated for every row in the
  // bin.
  // Note that we should then also recreate that variable in concatenate, to
  // ensure that those operations are reversible.
  auto binned = bin2<T>(var, indices, dims);
  std::map<Dim, Variable> coords;
  for (const auto &edge : edges)
    coords[edge.dims().inner()] = copy(edge);
  return {binned, std::move(coords)};
}
} // namespace

DataArray bucketby(const DataArrayConstView &array,
                   const std::vector<VariableConstView> &edges,
                   const std::vector<VariableConstView> &groups,
                   const std::vector<Dim> &dim_order) {
  if (array.dtype() == dtype<bucket<DataArray>>) {
    // TODO if rebinning, take into account masks (or fail)!
    auto bucketed =
        bucketby_impl<DataArray>(array.data(), edges, groups, dim_order);
    copy_metadata(array, bucketed);
    return bucketed;
  } else {
    const auto dim = array.dims().inner();
    // pretend existing binning along outermost binning dim to enable threading
    // TODO automatic setup with reasoble bin count
    const Dimensions dims(edges.front().dims().inner(), 4);
    const auto size = array.dims()[dim];
    auto begin = makeVariable<scipp::index>(dims);
    begin.values<scipp::index>()[0] = 0 * (size / 4);
    begin.values<scipp::index>()[1] = 1 * (size / 4);
    begin.values<scipp::index>()[2] = 2 * (size / 4);
    begin.values<scipp::index>()[3] = 3 * (size / 4);
    auto end = makeVariable<scipp::index>(dims);
    end.values<scipp::index>()[0] = 1 * (size / 4);
    end.values<scipp::index>()[1] = 2 * (size / 4);
    end.values<scipp::index>()[2] = 3 * (size / 4);
    end.values<scipp::index>()[3] = size;
    const auto indices = zip(begin, end);
    const auto tmp = make_non_owning_bins(indices, dim, array);
    // TODO lift metadata to outer if possible
    return bucketby_impl<DataArrayConstView>(tmp, edges, groups, dim_order);
    // return bucketby_impl(maybe_concat, edges, groups, dim_order);
  }
}

} // namespace scipp::dataset
