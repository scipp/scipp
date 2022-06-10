// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <numeric>
#include <set>

#include "scipp/core/element/bin.h"
#include "scipp/core/element/cumulative.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bin_detail.h"
#include "scipp/variable/bin_util.h"
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

#include "bin_detail.h"
#include "bins_util.h"
#include "dataset_operations_common.h"

using namespace scipp::variable::bin_detail;
using namespace scipp::dataset::bin_detail;

namespace scipp::dataset {

namespace {

template <class T> Variable as_subspan_view(T &&binned) {
  auto &&[indices, dim, buffer] = binned.template constituents<Variable>();
  if constexpr (std::is_const_v<std::remove_reference_t<T>>)
    return subspan_view(std::as_const(buffer), dim, indices);
  else
    return subspan_view(buffer, dim, indices);
}

auto make_range(const scipp::index begin, const scipp::index end,
                const scipp::index stride, const Dim dim) {
  return cumsum(broadcast(stride * units::none, {dim, (end - begin) / stride}),
                dim, CumSumMode::Exclusive);
}

void update_indices_by_binning(Variable &indices, const Variable &key,
                               const Variable &edges, const bool linspace) {
  const auto dim = edges.dims().inner();
  if (!indices.dims().includes(key.dims()))
    throw except::BinEdgeError(
        "Requested binning in dimension '" + to_string(dim) +
        "' but input contains a bin-edge coordinate with no corresponding "
        "event-coordinate. Provide an event coordinate or convert the "
        "bin-edge coordinate to a non-edge coordinate.");
  const auto &edge_view =
      is_bins(edges) ? as_subspan_view(edges) : subspan_view(edges, dim);
  if (linspace) {
    variable::transform_in_place(
        indices, key, edge_view,
        core::element::update_indices_by_binning_linspace,
        "scipp.bin.update_indices_by_binning_linspace");
  } else {
    variable::transform_in_place(
        indices, key, edge_view,
        core::element::update_indices_by_binning_sorted_edges,
        "scipp.bin.update_indices_by_binning_sorted_edges");
  }
}

template <class Index>
Variable groups_to_map(const Variable &var, const Dim dim) {
  return variable::transform(subspan_view(var, dim),
                             core::element::groups_to_map<Index>,
                             "scipp.bin.groups_to_map");
}

void update_indices_by_grouping(Variable &indices, const Variable &key,
                                const Variable &groups) {
  const auto dim = groups.dims().inner();
  const auto map = (indices.dtype() == dtype<int64_t>)
                       ? groups_to_map<int64_t>(groups, dim)
                       : groups_to_map<int32_t>(groups, dim);
  variable::transform_in_place(indices, key, map,
                               core::element::update_indices_by_grouping,
                               "scipp.bin.update_indices_by_grouping");
}

void update_indices_from_existing(Variable &indices, const Dim dim) {
  const scipp::index nbin = indices.dims()[dim];
  const auto index = make_range(0, nbin, 1, dim);
  variable::transform_in_place(indices, index, nbin * units::none,
                               core::element::update_indices_from_existing,
                               "scipp.bin.update_indices_from_existing");
}

/// `sub_bin` is a binned variable with sub-bin indices: new bins within bins
Variable bin_sizes(const Variable &sub_bin, const Variable &offset,
                   const Variable &nbin) {
  return variable::transform(
      as_subspan_view(sub_bin), offset, nbin, core::element::count_indices,
      "scipp.bin.bin_sizes"); // transform bins, not bin element
}

template <class T, class Builder>
auto bin(const Variable &data, const Variable &indices,
         const Builder &builder) {
  const auto dims = builder.dims();
  // Setup offsets within output bins, for every input bin. If rebinning occurs
  // along a dimension each output bin sees contributions from all input bins
  // along that dim.
  auto output_bin_sizes = bin_sizes(indices, builder.offsets(), builder.nbin());
  auto offsets = copy(output_bin_sizes);
  fill_zeros(offsets);
  // Not using cumsum along *all* dims, since some outer dims may be left
  // untouched (no rebin).
  std::vector<std::pair<Dim, scipp::index>> strategy;
  for (const auto dim : data.dims())
    if (dims.contains(dim))
      strategy.emplace_back(dim, data.dims()[dim]);
  // To avoid excessive memory consumption in intermediate results for
  // `output_bin_sizes` (in the loop below, computing sums and cumsums) we need
  // to ensure to handle the longest dimensions first,
  std::sort(strategy.begin(), strategy.end(),
            [](auto &&a, auto &&b) { return a.second > b.second; });
  for (const auto &item : strategy) {
    const auto dim = item.first;
    subbin_sizes_add_intersection(
        offsets, subbin_sizes_cumsum_exclusive(output_bin_sizes, dim));
    output_bin_sizes = sum(output_bin_sizes, dim);
  }
  // cumsum with bin dimension is last, since this corresponds to different
  // output bins, whereas the cumsum above handled different subbins of same
  // output bin, i.e., contributions of different input bins to some output bin.
  subbin_sizes_add_intersection(
      offsets, cumsum_exclusive_subbin_sizes(output_bin_sizes));
  Variable filtered_input_bin_size = sum_subbin_sizes(output_bin_sizes);
  auto end = cumsum(filtered_input_bin_size);
  const auto total_size =
      end.dims().volume() > 0 ? end.values<scipp::index>().as_span().back() : 0;
  end = broadcast(end, data.dims()); // required for some cases of rebinning
  const auto filtered_input_bin_ranges =
      zip(end - filtered_input_bin_size, end);

  // Perform actual binning step for data, all coords, all masks, ...
  const auto do_bin = [&](const auto &var) {
    if (!is_bins(var))
      return copy(var);
    const auto &[input_indices, buffer_dim, in_buffer] =
        var.template constituents<Variable>();
    static_cast<void>(input_indices);
    auto out = resize_default_init(in_buffer, buffer_dim, total_size);
    auto out_subspans =
        subspan_view(out, buffer_dim, filtered_input_bin_ranges);
    map_to_bins(out_subspans, as_subspan_view(var), offsets,
                as_subspan_view(indices));
    return out;
  };
  T out_buffer;
  if constexpr (std::is_same_v<T, Variable>)
    out_buffer = do_bin(data);
  else
    out_buffer = dataset::transform(bins_view<T>(data), do_bin);

  // Up until here the output was viewed with same bin index ranges as input.
  // Now switch to desired final bin indices.
  auto output_dims = merge(output_bin_sizes.dims(), dims);
  auto bin_sizes = makeVariable<scipp::index>(
      output_dims, units::none,
      Values(flatten_subbin_sizes(output_bin_sizes, dims.volume())));
  return std::tuple{std::move(out_buffer), std::move(bin_sizes)};
}

template <class T, class Meta> auto extract_unbinned(T &array, Meta meta) {
  const auto dim = array.dims().inner();
  using Key = typename std::decay_t<decltype(meta(array))>::key_type;
  std::vector<Key> to_extract;
  std::unordered_map<Key, Variable> extracted;
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
template <class Coords, class Masks, class Attrs>
DataArray add_metadata(std::tuple<DataArray, Variable> &&proto,
                       const Coords &coords, const Masks &masks,
                       const Attrs &attrs, const std::vector<Variable> &edges,
                       const std::vector<Variable> &groups,
                       const std::vector<Dim> &erase) {
  auto &[buffer, bin_sizes] = proto;
  bin_sizes = squeeze(bin_sizes, erase);
  const auto end = cumsum(bin_sizes);
  const auto buffer_dim = buffer.dims().inner();
  std::set<Dim> dims(erase.begin(), erase.end());
  const auto rebinned = [&](const auto &var) {
    for (const auto &dim : var.dims().labels())
      if (dims.count(dim) || var.dims().contains(buffer_dim))
        return true;
    return false;
  };
  auto out_coords = extract_unbinned(buffer, get_coords);
  for (const auto &c : {edges, groups})
    for (const auto &coord : c) {
      dims.emplace(coord.dims().inner());
      out_coords[coord.dims().inner()] = copy(coord);
    }
  for (const auto &[dim_, coord] : coords)
    if (!rebinned(coord) && !out_coords.count(dim_))
      out_coords[dim_] = copy(coord);
  auto out_masks = extract_unbinned(buffer, get_masks);
  for (const auto &[name, mask] : masks)
    if (!rebinned(mask))
      out_masks[name] = copy(mask);
  auto out_attrs = extract_unbinned(buffer, get_attrs);
  for (const auto &[dim_, coord] : attrs)
    if (!rebinned(coord) && !out_coords.count(dim_))
      out_attrs[dim_] = copy(coord);
  return DataArray{
      make_bins(zip(end - bin_sizes, end), buffer_dim, std::move(buffer)),
      std::move(out_coords), std::move(out_masks), std::move(out_attrs)};
}

class TargetBinBuilder {
  enum class AxisAction { Group, Bin, Existing, Join };

public:
  [[nodiscard]] const Dimensions &dims() const noexcept { return m_dims; }
  [[nodiscard]] const Variable &offsets() const noexcept { return m_offsets; }
  [[nodiscard]] const Variable &nbin() const noexcept { return m_nbin; }

  /// `bin_coords` may optionally be used to provide bin-based coords, e.g., for
  /// data that has prior grouping but did not retain the original group coord
  /// for every event.
  template <class CoordsT, class BinCoords = Coords>
  void build(Variable &indices, CoordsT &&coords, BinCoords &&bin_coords = {}) {
    const auto get_coord = [&](const Dim dim) {
      return coords.count(dim) ? coords[dim] : bin_coords.at(dim);
    };
    m_offsets = makeVariable<scipp::index>(Values{0}, units::none);
    m_nbin = dims().volume() * units::none;
    for (const auto &[action, dim, key] : m_actions) {
      if (action == AxisAction::Group)
        update_indices_by_grouping(indices, get_coord(dim), key);
      else if (action == AxisAction::Bin) {
        const auto linspace = all(islinspace(key, dim)).template value<bool>();
        // When binning along an existing dim with a coord (may be edges or
        // not), not all input bins can map to all output bins. The array of
        // subbin sizes that is normally created thus contains mainly zero
        // entries, e.g.,:
        // ---1
        // --11
        // --4-
        // 111-
        // 2---
        //
        // each row corresponds to an input bin
        // each column corresponds to an output bin
        // the example is for a single rebinned dim
        // `-` is 0
        //
        // In practice this array of sizes can become very large (many GByte of
        // memory) and has to be avoided. This is not just a performance issue.
        // We detect this case, pre select relevant output bins, and store the
        // sparse array in a specialized packed format, using the helper type
        // SubbinSizes.
        // Note that there is another source of memory consumption in the
        // algorithm, `indices`, containing the index of the target bin for
        // every input event. This is unrelated and varies independently,
        // depending on parameters of the input.
        if (key.ndim() == 1 && // index setup not implemented for this case
            bin_coords.count(dim) && m_offsets.dims().empty() &&
            bin_coords.at(dim).dims().contains(dim) &&
            allsorted(bin_coords.at(dim), dim)) {
          const auto &bin_coord = bin_coords.at(dim);
          const bool histogram =
              bin_coord.dims()[dim] == indices.dims()[dim] + 1;
          const auto begin =
              begin_edge(histogram ? left_edge(bin_coord) : bin_coord, key);
          const auto end = histogram ? end_edge(right_edge(bin_coord), key)
                                     : begin + 2 * units::none;
          const auto indices_ = zip(begin, end);
          const auto inner_volume = dims().volume() / dims()[dim] * units::none;
          // Number of non-zero entries (per "row" above)
          m_nbin = (end - begin - 1 * units::none) * inner_volume;
          // Offset to first non-zero entry (in "row" above)
          m_offsets = begin * inner_volume;
          // Mask out any output bin edges that need not be considered since
          // there is no overlap between given input and output bin.
          const auto masked_key = make_bins_no_validate(indices_, dim, key);
          update_indices_by_binning(indices, get_coord(dim), masked_key,
                                    linspace);
        } else {
          update_indices_by_binning(indices, get_coord(dim), key, linspace);
        }
      } else if (action == AxisAction::Existing) {
        // Similar to binning along an existing dim, if a dimension is simply
        // kept unchanged there is a 1:1 mapping from input to output dims. We
        // can thus avoid storing and processing a lot of length-0 contributions
        // to bins.
        // Note that this is only possible (in this simple manner) if there are
        // no other actions affecting output dimensions.
        if (m_offsets.dims().empty() && m_dims[dim] == m_dims.volume()) {
          // Offset to output bin tracked using base offset for input bins
          m_nbin = scipp::index{1} * units::none;
          m_offsets = make_range(0, m_dims[dim], 1, dim);
        } else {
          // Offset to output bin tracked in indices for individual events
          update_indices_from_existing(indices, dim);
        }
      } else if (action == AxisAction::Join) {
        ; // target bin 0 for all
      }
    }
  }

  [[nodiscard]] auto edges() const noexcept {
    std::vector<Variable> vars;
    for (const auto &[action, dim, key] : m_actions) {
      static_cast<void>(dim);
      if (action == AxisAction::Bin || action == AxisAction::Join)
        vars.emplace_back(key);
    }
    return vars;
  }

  [[nodiscard]] auto groups() const noexcept {
    std::vector<Variable> vars;
    for (const auto &[action, dim, key] : m_actions) {
      static_cast<void>(dim);
      if (action == AxisAction::Group)
        vars.emplace_back(key);
    }
    return vars;
  }

  void group(const Variable &groups) {
    const auto dim = groups.dims().inner();
    m_dims.addInner(dim, groups.dims()[dim]);
    m_actions.emplace_back(AxisAction::Group, dim, groups);
  }

  void bin(const Variable &edges) {
    const auto dim = edges.dims().inner();
    m_dims.addInner(dim, edges.dims()[dim] - 1);
    m_actions.emplace_back(AxisAction::Bin, dim, edges);
  }

  void existing(const Dim dim, const scipp::index size) {
    m_dims.addInner(dim, size);
    m_actions.emplace_back(AxisAction::Existing, dim, Variable{});
  }

  void join(const Dim dim, const Variable &coord) {
    m_dims.addInner(dim, 1);
    m_joined.emplace_back(concat(std::vector{min(coord), max(coord)}, dim));
    m_actions.emplace_back(AxisAction::Join, dim, m_joined.back());
  }

  // All input bins mapped to same output bin => "add" 0 everywhere
  void erase(const Dim dim) { m_dims.addInner(dim, 1); }

private:
  Dimensions m_dims;
  Variable m_offsets;
  Variable m_nbin;
  std::vector<std::tuple<AxisAction, Dim, Variable>> m_actions;
  std::vector<Variable> m_joined;
};

// Order is defined as:
// 1. Erase binning from any dimensions listed in erase
// 2. Any rebinned dim and dims inside the first rebinned dim, in the order of
// appearance in array.
// 3. All new grouped dims.
// 4. All new binned dims.
template <class Coords>
auto axis_actions(const Variable &data, const Coords &coords,
                  const std::vector<Variable> &edges,
                  const std::vector<Variable> &groups,
                  const std::vector<Dim> &erase) {
  TargetBinBuilder builder;
  for (const auto dim : erase) {
    builder.erase(dim);
  }

  constexpr auto get_dims = [](const auto &coords_) {
    Dimensions dims;
    for (const auto &coord : coords_)
      dims.addInner(coord.dims().inner(), 1);
    return dims;
  };
  auto edges_dims = get_dims(edges);
  auto groups_dims = get_dims(groups);
  // If we rebin a dimension that is not the inner dimension of the input, we
  // also need to handle bin contents from all dimensions inside the rebinned
  // one, even if the grouping/binning along this dimension is unchanged.
  bool rebin = false;
  const auto dims = data.dims();
  for (const auto dim : dims.labels()) {
    if (edges_dims.contains(dim) || groups_dims.contains(dim))
      rebin = true;
    if (groups_dims.contains(dim)) {
      builder.group(groups[groups_dims.index(dim)]);
    } else if (edges_dims.contains(dim)) {
      builder.bin(edges[edges_dims.index(dim)]);
    } else if (rebin) {
      if (coords.count(dim) && coords.at(dim).dims().ndim() != 1)
        throw except::DimensionError(
            "2-D coordinate " + to_string(coords.at(dim)) +
            " conflicting with (re)bin of outer dimension. Try specifying new "
            "aligned (1-D) edges for dimension '" +
            to_string(dim) + "' with the `edges` option of `bin`.");
      builder.existing(dim, data.dims()[dim]);
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

template <class T> class TargetBins {
public:
  TargetBins(const Variable &var, const Dimensions &dims) {
    // In some cases all events in an input bin map to the same output, but
    // right now bin<> cannot handle this and requires target bin indices for
    // every bin element.
    const auto &[begin_end, dim, buffer] = var.constituents<T>();
    m_target_bins_buffer =
        (dims.volume() > std::numeric_limits<int32_t>::max())
            ? makeVariable<int64_t>(buffer.dims(), units::none)
            : makeVariable<int32_t>(buffer.dims(), units::none);
    m_target_bins = make_bins_no_validate(begin_end, dim, m_target_bins_buffer);
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
template <class T> Variable concat_bins(const Variable &var, const Dim dim) {
  TargetBinBuilder builder;
  builder.erase(dim);
  TargetBins<T> target_bins(var, builder.dims());

  builder.build(*target_bins, std::map<Dim, Variable>{});
  auto [buffer, bin_sizes] = bin<T>(var, *target_bins, builder);
  bin_sizes = squeeze(bin_sizes, scipp::span{&dim, 1});
  const auto end = cumsum(bin_sizes);
  const auto buffer_dim = buffer.dims().inner();
  return make_bins(zip(end - bin_sizes, end), buffer_dim, std::move(buffer));
}
template Variable concat_bins<Variable>(const Variable &, const Dim);
template Variable concat_bins<DataArray>(const Variable &, const Dim);

/// Implementation of groupby.bins.concatenate
///
/// If `array` has unaligned, i.e., not 1-D, coords conflicting with the
/// reduction dimension, any binning along the dimensions of the conflicting
/// coords is removed. It is replaced by a single bin along that dimension, with
/// bin edges given my min and max of the old coord.
DataArray groupby_concat_bins(const DataArray &array, const Variable &edges,
                              const Variable &groups, const Dim reductionDim) {
  TargetBinBuilder builder;
  if (edges.is_valid())
    builder.bin(edges);
  if (groups.is_valid())
    builder.group(groups);
  builder.erase(reductionDim);
  const auto dims = array.dims();
  for (const auto &dim : dims.labels())
    if (array.meta().contains(dim)) {
      if (array.meta()[dim].dims().ndim() != 1 &&
          array.meta()[dim].dims().contains(reductionDim))
        builder.join(dim, array.meta()[dim]);
      else if (dim != reductionDim)
        builder.existing(dim, array.dims()[dim]);
    }

  const auto masked =
      hide_masked(array.data(), array.masks(), builder.dims().labels());
  TargetBins<DataArray> target_bins(masked, builder.dims());
  builder.build(*target_bins, array.meta());
  // Note: Unlike in the other cases below we do not call
  // `drop_grouped_event_coords` here. Grouping is based on a bin-coord rather
  // than event-coord so we do not touch the latter.
  return add_metadata(bin<DataArray>(masked, *target_bins, builder),
                      array.coords(), array.masks(), array.attrs(),
                      builder.edges(), builder.groups(), {reductionDim});
}

namespace {
void validate_bin_args(const DataArray &array,
                       const std::vector<Variable> &edges,
                       const std::vector<Variable> &groups) {
  if ((is_bins(array) &&
       std::get<2>(array.data().constituents<DataArray>()).dims().ndim() > 1) ||
      (!is_bins(array) && array.dims().ndim() > 1)) {
    throw except::BinnedDataError(
        "Binning is only implemented for 1-dimensional data. Consider using "
        "groupby, it might be able to do what you need.");
  }
  if (edges.empty() && groups.empty())
    throw std::invalid_argument(
        "Arguments 'edges' and 'groups' of scipp.bin are "
        "both empty. At least one must be set.");
  for (const auto &edge : edges) {
    const auto dim = edge.dims().inner();
    if (edge.dims()[dim] < 2)
      throw except::BinEdgeError("Not enough bin edges in dim " +
                                 to_string(dim) + ". Need at least 2.");
    if (!allsorted(edge, dim))
      throw except::BinEdgeError("Bin edges in dim " + to_string(dim) +
                                 " must be sorted.");
  }
}

auto drop_grouped_event_coords(const Variable &data,
                               const std::vector<Variable> &groups) {
  auto [indices, dim, buffer] = data.constituents<DataArray>();
  // Do not preserve event coords used for grouping since this is redundant
  // information and leads to waste of memory and compute in follow-up
  // operations.
  for (const auto &var : groups)
    if (buffer.coords().contains(var.dims().inner()))
      buffer.coords().erase(var.dims().inner());
  return make_bins_no_validate(indices, dim, buffer);
}

} // namespace

DataArray bin(const DataArray &array, const std::vector<Variable> &edges,
              const std::vector<Variable> &groups,
              const std::vector<Dim> &erase) {
  validate_bin_args(array, edges, groups);
  const auto &data = array.data();
  const auto &coords = array.coords();
  const auto &meta = array.meta();
  const auto &masks = array.masks();
  const auto &attrs = array.attrs();
  if (data.dtype() == dtype<core::bin<DataArray>>) {
    return bin(data, coords, masks, attrs, edges, groups, erase);
  } else {
    // Pretend existing binning along outermost binning dim to enable threading
    const auto dim = data.dims().inner();
    const auto size = std::max(scipp::index(1), data.dims()[dim]);
    // TODO automatic setup with reasonable bin count
    const auto stride = std::max(scipp::index(1), size / 24);
    auto begin = make_range(0, size, stride,
                            groups.empty() ? edges.front().dims().inner()
                                           : groups.front().dims().inner());
    auto end = begin + stride * units::none;
    end.values<scipp::index>().as_span().back() = data.dims()[dim];
    const auto indices = zip(begin, end);
    const auto tmp = make_bins_no_validate(indices, dim, array);
    auto target_bins_buffer =
        (data.dims().volume() > std::numeric_limits<int32_t>::max())
            ? makeVariable<int64_t>(data.dims(), units::none)
            : makeVariable<int32_t>(data.dims(), units::none);
    auto builder = axis_actions(data, meta, edges, groups, erase);
    builder.build(target_bins_buffer, meta);
    const auto target_bins =
        make_bins_no_validate(indices, dim, target_bins_buffer);
    return add_metadata(bin<DataArray>(drop_grouped_event_coords(tmp, groups),
                                       target_bins, builder),
                        coords, masks, attrs, builder.edges(), builder.groups(),
                        erase);
  }
}

/// Implementation of a generic binning algorithm.
///
/// The overall approach of this is as follows:
/// 1. Find target bin index for every input event (bin entry)
/// 2. Next, we conceptually want to do
///        for(i < events.size())
///          target_bin[bin_index[i]].push_back(events[i])
///    However, scipp's data layout for event data is a single 1-D array, and
///    not a list of vector, i.e., the conceptual line above does not work
///    directly. We need to obtain offsets into the 1-D array first, roughly:
///        bin_sizes = count(bin_index) // number of events per target bin
///        bin_offset = cumsum(bin_sizes) - bin_sizes
/// 3. Copy from input to output bin, based on offset
template <class Coords, class Masks, class Attrs>
DataArray bin(const Variable &data, const Coords &coords, const Masks &masks,
              const Attrs &attrs, const std::vector<Variable> &edges,
              const std::vector<Variable> &groups,
              const std::vector<Dim> &erase) {
  const auto meta = attrs.merge_from(coords);
  auto builder = axis_actions(data, meta, edges, groups, erase);
  const auto masked = hide_masked(data, masks, builder.dims().labels());
  TargetBins<DataArray> target_bins(masked, builder.dims());
  builder.build(*target_bins, bins_view<DataArray>(masked).meta(), meta);
  return add_metadata(bin<DataArray>(drop_grouped_event_coords(masked, groups),
                                     *target_bins, builder),
                      coords, masks, attrs, builder.edges(), builder.groups(),
                      erase);
}

} // namespace scipp::dataset
