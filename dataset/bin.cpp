// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>

#include "scipp/core/element/bin.h"
#include "scipp/core/element/cumulative.h"
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

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/except.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

namespace {

auto make_range(const scipp::index begin, const scipp::index end,
                const scipp::index stride, const Dim dim) {
  return exclusive_scan(
      broadcast(stride * units::one, {dim, (end - begin) / stride}), dim);
}

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

void update_indices_from_existing(const VariableView &indices, const Dim dim) {
  const scipp::index nbin = indices.dims()[dim];
  const auto index = make_range(0, nbin, 1, dim);
  variable::transform_in_place(indices, index, nbin * units::one,
                               core::element::update_indices_from_existing);
}

template <class T> Variable as_subspan_view(T &&binned) {
  if (binned.dtype() == dtype<core::bin<Variable>>) {
    auto &&[indices, dim, buffer] =
        binned.template constituents<core::bin<Variable>>();
    return subspan_view(buffer, dim, indices);
  } else if (binned.dtype() == dtype<core::bin<VariableView>>) {
    auto &&[indices, dim, buffer] =
        binned.template constituents<core::bin<VariableView>>();
    return subspan_view(buffer, dim, indices);
  } else {
    auto &&[indices, dim, buffer] =
        binned.template constituents<core::bin<VariableConstView>>();
    return subspan_view(buffer, dim, indices);
  }
}

/// indices is a binned variable with sub-bin indices, i.e., new bins within
/// bins
Variable bin_sizes(const VariableConstView &sub_bin, const scipp::index nbin) {
  const auto nbins = broadcast(nbin * units::one, sub_bin.dims());
  auto sizes = resize(sub_bin, nbins);
  buckets::reserve(sizes, nbins);
  variable::transform_in_place(
      as_subspan_view(sizes), as_subspan_view(sub_bin),
      core::element::count_indices); // transform bins, not bin element
  return sizes;
}

Variable exclusive_scan_bins(const VariableView &var) {
  Variable out(var);
  transform_in_place(as_subspan_view(out), core::element::exclusive_scan_bins);
  return out;
}

template <class T>
auto bin(const VariableConstView &data, const VariableConstView &indices,
         const Dimensions &dims) {
  // Setup offsets within output bins, for every input bin. If rebinning occurs
  // along a dimension each output bin sees contributions from all input bins
  // along that dim.
  const auto nbin = dims.volume();
  auto output_bin_sizes = bin_sizes(indices, nbin);
  auto offsets = output_bin_sizes;
  fill_zeros(offsets);
  for (const auto dim : data.dims().labels())
    if (dims.contains(dim)) {
      offsets += exclusive_scan(output_bin_sizes, dim);
      output_bin_sizes = sum(output_bin_sizes, dim);
    }
  offsets += exclusive_scan_bins(output_bin_sizes);
  Variable filtered_input_bin_size = buckets::sum(output_bin_sizes);
  auto [begin, total_size] = sizes_to_begin(filtered_input_bin_size);
  begin = broadcast(begin, data.dims()); // required for some cases of rebinning
  const auto filtered_input_bin_ranges =
      zip(begin, begin + filtered_input_bin_size);

  // Perform actual binning step for data, all coords, all masks, ...
  auto out_buffer =
      dataset::transform(bins_view<T>(data), [&, N = total_size](auto &&var) {
        if (!is_buckets(var))
          return std::move(var);
        const auto &[input_indices, buffer_dim, in_buffer] =
            var.template constituents<core::bin<VariableConstView>>();
        auto out = resize_default_init(in_buffer, buffer_dim, N);
        transform_in_place(
            subspan_view(out, buffer_dim, filtered_input_bin_ranges),
            as_subspan_view(std::as_const(offsets)), as_subspan_view(var),
            as_subspan_view(indices), core::element::bin);
        return out;
      });

  // Up until here the output was viewed with same bin index ranges as input.
  // Now switch to desired final bin indices.
  auto output_dims = merge(output_bin_sizes.dims(), dims);
  const auto bin_sizes =
      reshape(std::get<2>(output_bin_sizes.constituents<core::bin<Variable>>()),
              output_dims);
  std::tie(begin, total_size) = sizes_to_begin(bin_sizes);
  const auto dim = out_buffer.dims().inner();
  return make_bins(zip(begin, begin + bin_sizes), dim, std::move(out_buffer));
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
enum class AxisAction { Group, Bin, Existing };
template <class T>
Variable
bin_impl(const VariableConstView &var,
         const std::vector<std::tuple<AxisAction, Dim, VariableConstView>>
             &actions) {
  const auto &[begin_end, buffer_dim, array] = var.constituents<core::bin<T>>();
  const auto input_bins = bins_view<T>(var);
  auto indices = make_bins(copy(begin_end), buffer_dim,
                           makeVariable<scipp::index>(array.dims()));
  Dimensions dims;
  for (const auto &[type, dim, coord] : actions) {
    if (type == AxisAction::Group) {
      update_indices_by_grouping(indices, input_bins.coords()[dim], coord);
      dims.addInner(dim, coord.dims()[dim]);
    } else if (type == AxisAction::Bin) {
      update_indices_by_binning(indices, input_bins.coords()[dim], coord);
      dims.addInner(dim, coord.dims()[dim] - 1);
    } else {
      update_indices_from_existing(indices, dim);
      dims.addInner(dim, var.dims()[dim]);
    }
  }
  return bin<T>(var, indices, dims);
}

DataArray add_metadata(Variable &&binned, const DataArrayConstView &array,
                       const std::vector<VariableConstView> &edges,
                       const std::vector<VariableConstView> &groups) {
  // TODO We probably want to omit the coord used for grouping in the non-edge
  // case, since it just contains the same value duplicated for every row in the
  // bin.
  // Note that we should then also recreate that variable in concatenate, to
  // ensure that those operations are reversible.
  // TODO lift metadata to outer if possible
  std::map<Dim, Variable> coords;
  for (const auto &edge : edges)
    coords[edge.dims().inner()] = copy(edge);
  const auto &[begin_end, buffer_dim, buffer] =
      binned.constituents<core::bin<DataArray>>();
  for (const auto &[dim, coord] : array.aligned_coords())
    if (!coords.count(dim) && !coord.dims().contains(buffer_dim))
      coords[dim] = copy(coord);
  return {std::move(binned), std::move(coords)};
}

// Order is defined as:
// 1. Any rebinned dim and dims inside the first rebinned dim, in the order of
// appearance in array.
// 2. All new grouped dims.
// 3. All new binned dims.
auto axis_actions(const VariableConstView &var,
                  const std::vector<VariableConstView> &edges,
                  const std::vector<VariableConstView> &groups) {
  constexpr auto get_dims = [](const auto &coords) {
    Dimensions dims;
    for (const auto &coord : coords)
      dims.addInner(coord.dims().inner(), 1);
    return dims;
  };
  auto edges_dims = get_dims(edges);
  auto groups_dims = get_dims(groups);
  std::vector<std::tuple<AxisAction, Dim, VariableConstView>> axes;
  // If we rebin a dimension that is not the inner dimension of the input, we
  // also need to handle bin contents from all dimensions inside the rebinned
  // one, even if the grouping/binning along this dimension is unchanged.
  bool rebin = false;
  const auto dims = var.dims();
  for (const auto dim : dims.labels()) {
    if (edges_dims.contains(dim) || groups_dims.contains(dim))
      rebin = true;
    if (groups_dims.contains(dim))
      axes.emplace_back(AxisAction::Group, dim, groups[groups_dims.index(dim)]);
    else if (edges_dims.contains(dim))
      axes.emplace_back(AxisAction::Bin, dim, edges[edges_dims.index(dim)]);
    else if (rebin)
      axes.emplace_back(AxisAction::Existing, dim, VariableConstView{});
  }
  for (const auto &group : groups)
    if (!dims.contains(group.dims().inner()))
      axes.emplace_back(AxisAction::Group, group.dims().inner(), group);
  for (const auto &edge : edges)
    if (!dims.contains(edge.dims().inner()))
      axes.emplace_back(AxisAction::Bin, edge.dims().inner(), edge);
  return axes;
}

} // namespace

DataArray bin(const DataArrayConstView &array,
              const std::vector<VariableConstView> &edges,
              const std::vector<VariableConstView> &groups) {
  Variable binned;
  const auto actions = axis_actions(array.data(), edges, groups);
  if (array.dtype() == dtype<core::bin<DataArray>>) {
    // TODO if rebinning, take into account masks (or fail)!
    binned = bin_impl<DataArray>(array.data(), actions);
  } else {
    // Pretend existing binning along outermost binning dim to enable threading
    const auto dim = array.dims().inner();
    const auto size = std::max(scipp::index(1), array.dims()[dim]);
    // TODO automatic setup with reasonable bin count
    const auto stride = std::max(scipp::index(1), size / 24);
    auto begin = make_range(0, size, stride, edges.front().dims().inner());
    auto end = begin + stride * units::one;
    end.values<scipp::index>().as_span().back() = array.dims()[dim];
    const auto indices = zip(begin, end);
    const auto tmp = make_non_owning_bins(indices, dim, array);
    binned = bin_impl<DataArrayConstView>(tmp, actions);
  }
  return add_metadata(std::move(binned), array, edges, groups);
}

} // namespace scipp::dataset
