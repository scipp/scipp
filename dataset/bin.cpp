// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>
#include <set>

#include "scipp/core/element/bin.h"
#include "scipp/core/element/cumulative.h"
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
  return cumsum(broadcast(stride * units::one, {dim, (end - begin) / stride}),
                dim, CumSumMode::Exclusive);
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
      offsets += cumsum(output_bin_sizes, dim, CumSumMode::Exclusive);
      output_bin_sizes = sum(output_bin_sizes, dim);
    }
  offsets += cumsum_bins(output_bin_sizes, CumSumMode::Exclusive);
  Variable filtered_input_bin_size = buckets::sum(output_bin_sizes);
  auto end = cumsum(filtered_input_bin_size);
  const auto total_size = end.values<scipp::index>().as_span().back();
  end = broadcast(end, data.dims()); // required for some cases of rebinning
  const auto filtered_input_bin_ranges =
      zip(end - filtered_input_bin_size, end);

  // Perform actual binning step for data, all coords, all masks, ...
  auto out_buffer = dataset::transform(bins_view<T>(data), [&](auto &&var) {
    if (!is_buckets(var))
      return std::move(var);
    const auto &[input_indices, buffer_dim, in_buffer] =
        var.template constituents<core::bin<VariableConstView>>();
    auto out = resize_default_init(in_buffer, buffer_dim, total_size);
    transform_in_place(subspan_view(out, buffer_dim, filtered_input_bin_ranges),
                       as_subspan_view(std::as_const(offsets)),
                       as_subspan_view(var), as_subspan_view(indices),
                       core::element::bin);
    return out;
  });

  // Up until here the output was viewed with same bin index ranges as input.
  // Now switch to desired final bin indices.
  auto output_dims = merge(output_bin_sizes.dims(), dims);
  auto bin_sizes =
      reshape(std::get<2>(output_bin_sizes.constituents<core::bin<Variable>>()),
              output_dims);
  return std::tuple{std::move(out_buffer), std::move(bin_sizes)};
}

constexpr static auto get_coords = [](auto &a) { return a.coords(); };
constexpr static auto get_attrs = [](auto &a) { return a.attrs(); };
constexpr static auto get_masks = [](auto &a) { return a.masks(); };

template <class T, class Meta> auto extract_unbinned(T &array, Meta meta) {
  const auto dim = array.dims().inner();
  std::vector<typename decltype(meta(array))::key_type> to_extract;
  std::map<typename decltype(meta(array))::key_type, Variable> extracted;
  const auto view = meta(array);
  // WARNING: Do not use `view` while extracting, `extract` invalidates it!
  std::copy_if(
      view.keys_begin(), view.keys_end(), std::back_inserter(to_extract),
      [&](const auto &key) { return !view[key].dims().contains(dim); });
  std::transform(to_extract.begin(), to_extract.end(),
                 std::inserter(extracted, extracted.end()),
                 [&](const auto &key) {
                   return std::pair(key, meta(array).extract(key));
                 });
  return extracted;
}

/// Combine meta data from buffer and input data array and create final output
/// data array with binned data.
/// - Meta data that does not depend on the buffer dim is lifted to the output
///   array.
/// - Any meta data depending on rebinned dimensions is dropped since it becomes
///   meaningless. Note that rebinned masks have been applied before the binning
///   step.
/// - If rebinning, existing meta data along unchanged dimensions is preserved.
DataArray add_metadata(std::tuple<DataArray, Variable> &&proto,
                       const DataArrayConstView &array,
                       const std::vector<VariableConstView> &edges,
                       const std::vector<VariableConstView> &groups,
                       const std::vector<Dim> &erase) {
  auto &[buffer, bin_sizes] = proto;
  squeeze(bin_sizes, erase);
  const auto end = cumsum(bin_sizes);
  const auto buffer_dim = buffer.dims().inner();
  // TODO We probably want to omit the coord used for grouping in the non-edge
  // case, since it just contains the same value duplicated for every row in the
  // bin.
  // Note that we should then also recreate that variable in concatenate, to
  // ensure that those operations are reversible.
  std::set<Dim> dims(erase.begin(), erase.end());
  const auto rebinned = [&](const auto &var) {
    for (const auto &dim : var.dims().labels())
      if (dims.count(dim) || var.dims().contains(buffer_dim))
        return true;
    return false;
  };
  auto coords = extract_unbinned(buffer, get_coords);
  for (const auto &c : {edges, groups})
    for (const auto &coord : c) {
      dims.emplace(coord.dims().inner());
      coords[coord.dims().inner()] = copy(coord);
    }
  for (const auto &[dim, coord] : array.coords())
    if (!rebinned(coord))
      coords[dim] = copy(coord);
  auto masks = extract_unbinned(buffer, get_masks);
  for (const auto &[name, mask] : array.masks())
    if (!rebinned(mask))
      masks[name] = copy(mask);
  auto attrs = extract_unbinned(buffer, get_attrs);
  for (const auto &[dim, coord] : array.attrs())
    if (!rebinned(coord))
      attrs[dim] = copy(coord);
  return {make_bins(zip(end - bin_sizes, end), buffer_dim, std::move(buffer)),
          std::move(coords), std::move(masks), std::move(attrs)};
}

class TargetBinBuilder {
  enum class AxisAction { Group, Bin, Existing, Join };

public:
  const Dimensions &dims() const noexcept { return m_dims; }

  template <class Coords>
  void build(const VariableView &indices, Coords &&coords) const {
    for (const auto &[action, dim, key] : m_actions) {
      if (action == AxisAction::Group)
        update_indices_by_grouping(indices, coords[dim], key);
      else if (action == AxisAction::Bin)
        update_indices_by_binning(indices, coords[dim], key);
      else if (action == AxisAction::Existing)
        update_indices_from_existing(indices, dim);
      else if (action == AxisAction::Join) {
        ; // target bin 0 for all
      }
    }
  }

  auto edges() const noexcept {
    std::vector<VariableConstView> vars;
    for (const auto &[action, dim, key] : m_actions)
      if (action == AxisAction::Bin || action == AxisAction::Join)
        vars.emplace_back(key);
    return vars;
  }

  auto groups() const noexcept {
    std::vector<VariableConstView> vars;
    for (const auto &[action, dim, key] : m_actions)
      if (action == AxisAction::Group)
        vars.emplace_back(key);
    return vars;
  }

  void group(const VariableConstView &groups) {
    const auto dim = groups.dims().inner();
    m_dims.addInner(dim, groups.dims()[dim]);
    m_actions.emplace_back(AxisAction::Group, dim, groups);
  }

  void bin(const VariableConstView &edges) {
    const auto dim = edges.dims().inner();
    m_dims.addInner(dim, edges.dims()[dim] - 1);
    m_actions.emplace_back(AxisAction::Bin, dim, edges);
  }

  void existing(const Dim dim, const scipp::index size) {
    m_dims.addInner(dim, size);
    m_actions.emplace_back(AxisAction::Existing, dim, VariableConstView{});
  }

  void join(const Dim dim, const VariableConstView &coord) {
    m_dims.addInner(dim, 1);
    m_joined.emplace_back(concatenate(min(coord), max(coord), dim));
    m_actions.emplace_back(AxisAction::Join, dim, m_joined.back());
  }

  // All input bins mapped to same output bin => "add" 0 everywhere
  void erase(const Dim dim) { m_dims.addInner(dim, 1); }

private:
  Dimensions m_dims;
  std::vector<std::tuple<AxisAction, Dim, VariableConstView>> m_actions;
  std::vector<Variable> m_joined;
};

// Order is defined as:
// 1. Any rebinned dim and dims inside the first rebinned dim, in the order of
// appearance in array.
// 2. All new grouped dims.
// 3. All new binned dims.
auto axis_actions(const DataArrayConstView &array,
                  const std::vector<VariableConstView> &edges,
                  const std::vector<VariableConstView> &groups) {
  TargetBinBuilder builder;
  constexpr auto get_dims = [](const auto &coords) {
    Dimensions dims;
    for (const auto &coord : coords)
      dims.addInner(coord.dims().inner(), 1);
    return dims;
  };
  auto edges_dims = get_dims(edges);
  auto groups_dims = get_dims(groups);
  // If we rebin a dimension that is not the inner dimension of the input, we
  // also need to handle bin contents from all dimensions inside the rebinned
  // one, even if the grouping/binning along this dimension is unchanged.
  bool rebin = false;
  const auto dims = array.dims();
  for (const auto dim : dims.labels()) {
    if (edges_dims.contains(dim) || groups_dims.contains(dim))
      rebin = true;
    if (groups_dims.contains(dim)) {
      builder.group(groups[groups_dims.index(dim)]);
    } else if (edges_dims.contains(dim)) {
      builder.bin(edges[edges_dims.index(dim)]);
    } else if (rebin) {
      if (array.coords().contains(dim) &&
          array.coords()[dim].dims().ndim() != 1)
        throw except::DimensionError(
            "2-D coordinate " + to_string(array.coords()[dim]) +
            " conflicting with (re)bin of outer dimension. Try specifying new "
            "aligned (1-D) edges for dimension '" +
            to_string(dim) + "' with the `edges` option of `bin`.");
      builder.existing(dim, array.dims()[dim]);
    }
  }
  for (const auto &group : groups)
    if (!dims.contains(group.dims().inner()))
      builder.group(group);
  for (const auto &edge : edges)
    if (!dims.contains(edge.dims().inner()))
      builder.bin(edge);
  return builder;
}

auto hide_masked(const DataArrayConstView &array, const Dimensions &dims) {
  const auto &[begin_end, buffer_dim, buffer] =
      array.data().constituents<core::bin<DataArray>>();
  auto [begin, end] = unzip(begin_end);
  for (const auto dim : dims.labels()) {
    auto mask = irreducible_mask(array.masks(), dim);
    if (mask) {
      begin *= ~mask;
      end *= ~mask;
    }
  }
  return make_non_owning_bins(zip(begin, end), buffer_dim, buffer);
}

template <class T> class TargetBins {
public:
  TargetBins(const VariableConstView &var) {
    // In some cases all events in an input bin map to the same output, but
    // right now bin<> cannot handle this and requires target bin indices for
    // every bin element.
    const auto &[begin_end, dim, buffer] = var.constituents<core::bin<T>>();
    m_target_bins_buffer = makeVariable<scipp::index>(buffer.dims());
    m_target_bins = make_non_owning_bins(begin_end, dim,
                                         VariableView(m_target_bins_buffer));
  }
  auto &operator*() noexcept { return m_target_bins; }

private:
  Variable m_target_bins_buffer;
  Variable m_target_bins;
};

} // namespace

/// Reduce a dimension by concatenating bin contents of all bins along a
/// dimension.
///
/// This is used to implement `concatenate(var, dim)`.
template <class T>
Variable concat_bins(const VariableConstView &var, const Dim dim) {
  TargetBinBuilder builder;
  builder.erase(dim);
  TargetBins<T> target_bins(var);
  builder.build(*target_bins, std::map<Dim, Variable>{});
  auto [buffer, bin_sizes] = bin<DataArray>(var, *target_bins, builder.dims());
  squeeze(bin_sizes, {dim});
  const auto end = cumsum(bin_sizes);
  const auto buffer_dim = buffer.dims().inner();
  return make_bins(zip(end - bin_sizes, end), buffer_dim, std::move(buffer));
}
template Variable concat_bins<Variable>(const VariableConstView &, const Dim);
template Variable concat_bins<DataArray>(const VariableConstView &, const Dim);

/// Implementation of groupby.bins.concatenate
///
/// If `array` has unaligned, i.e., not 1-D, coords conflicting with the
/// reduction dimension, any binning along the dimensions of the conflicting
/// coords is removed. It is replaced by a single bin along that dimension, with
/// bin edges given my min and max of the old coord.
DataArray groupby_concat_bins(const DataArrayConstView &array,
                              const VariableConstView &edges,
                              const VariableConstView &groups,
                              const Dim reductionDim) {
  TargetBinBuilder builder;
  if (edges)
    builder.bin(edges);
  if (groups)
    builder.group(groups);
  builder.erase(reductionDim);
  const auto dims = array.dims();
  for (const auto &dim : dims.labels())
    if (array.coords().contains(dim) &&
        array.coords()[dim].dims().ndim() != 1 &&
        array.coords()[dim].dims().contains(reductionDim))
      builder.join(dim, array.coords()[dim]);

  const auto masked = hide_masked(array, builder.dims());
  TargetBins<DataArrayConstView> target_bins(masked);
  builder.build(*target_bins, array.coords());
  return add_metadata(
      bin<DataArrayConstView>(masked, *target_bins, builder.dims()), array,
      builder.edges(), builder.groups(), {reductionDim});
}

DataArray bin(const DataArrayConstView &array,
              const std::vector<VariableConstView> &edges,
              const std::vector<VariableConstView> &groups) {
  std::tuple<DataArray, Variable> proto;
  auto builder = axis_actions(array, edges, groups);
  if (array.dtype() == dtype<core::bin<DataArray>>) {
    const auto masked = hide_masked(array, builder.dims());
    TargetBins<DataArrayConstView> target_bins(masked);
    builder.build(*target_bins, bins_view<DataArrayConstView>(masked).coords());
    proto = bin<DataArrayConstView>(masked, *target_bins, builder.dims());
  } else {
    // Pretend existing binning along outermost binning dim to enable threading
    const auto dim = array.dims().inner();
    const auto size = std::max(scipp::index(1), array.dims()[dim]);
    // TODO automatic setup with reasonable bin count
    const auto stride = std::max(scipp::index(1), size / 24);
    auto begin = make_range(0, size, stride,
                            groups.empty() ? edges.front().dims().inner()
                                           : groups.front().dims().inner());
    auto end = begin + stride * units::one;
    end.values<scipp::index>().as_span().back() = array.dims()[dim];
    const auto indices = zip(begin, end);
    const auto tmp = make_non_owning_bins(indices, dim, array);
    auto target_bins_buffer = makeVariable<scipp::index>(array.dims());
    builder.build(target_bins_buffer, array.coords());
    const auto target_bins = make_non_owning_bins(
        indices, dim, VariableConstView(target_bins_buffer));
    proto = bin<DataArrayConstView>(tmp, target_bins, builder.dims());
  }
  return add_metadata(std::move(proto), array, builder.edges(),
                      builder.groups(), {});
}

} // namespace scipp::dataset
