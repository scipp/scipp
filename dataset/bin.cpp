// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <iostream>
#include <numeric>
#include <set>

#include "scipp/core/element/bin.h"
#include "scipp/core/element/cumulative.h"

#include "scipp/variable/arithmetic.h"
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

template <class Index>
Variable groups_to_map(const VariableConstView &var, const Dim dim) {
  return variable::transform(subspan_view(var, dim),
                             core::element::groups_to_map<Index>);
}

void update_indices_by_grouping(const VariableView &indices,
                                const VariableConstView &key,
                                const VariableConstView &groups) {
  const auto dim = groups.dims().inner();
  const auto map = (indices.dtype() == dtype<int64_t>)
                       ? groups_to_map<int64_t>(groups, dim)
                       : groups_to_map<int32_t>(groups, dim);
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
  const auto end = cumsum(broadcast(nbin * units::one, sub_bin.dims()));
  const auto dim = variable::variableFactory().elem_dim(sub_bin);
  auto sizes = make_bins(
      zip(end - nbin * units::one, end), dim,
      makeVariable<scipp::index>(Dims{dim}, Shape{end.dims().volume() * nbin}));
  variable::transform_in_place(
      as_subspan_view(sizes), as_subspan_view(sub_bin),
      core::element::count_indices); // transform bins, not bin element
  return sizes;
}

/// indices is a binned variable with sub-bin indices, i.e., new bins within
/// bins
Variable bin_sizes2(const VariableConstView &sub_bin,
                    const VariableConstView &offset,
                    const VariableConstView &nbin) {
  return variable::transform(
      as_subspan_view(sub_bin), offset, nbin,
      core::element::count_indices2); // transform bins, not bin element
}

template <class T, class Builder>
auto bin(const VariableConstView &data, const VariableConstView &indices,
         const Builder &builder) {
  const auto dims = builder.dims();
  // Setup offsets within output bins, for every input bin. If rebinning occurs
  // along a dimension each output bin sees contributions from all input bins
  // along that dim.
  Variable output_bin_sizes =
      bin_sizes2(indices, builder.offsets, builder.nbin);
  std::cout << output_bin_sizes;
  auto offsets = output_bin_sizes;
  fill_zeros(offsets);
  // Not using cumsum along *all* dims, since some outer dims may be left
  // untouched (no rebin).
  for (const auto dim : data.dims().labels())
    if (dims.contains(dim)) {
      offsets += cumsum(output_bin_sizes, dim, CumSumMode::Exclusive);
      output_bin_sizes = sum(output_bin_sizes, dim);
    }
  std::cout << output_bin_sizes;
  std::cout << offsets;
  // cumsum with bin dimension is last, since this corresponds to different
  // output bins, whereas the cumsum above handled different subbins of same
  // output bin, i.e., contributions of different input bins to some output bin.
  // offsets += cumsum_bins(output_bin_sizes, CumSumMode::Exclusive);
  // Variable filtered_input_bin_size = buckets::sum(output_bin_sizes);
  offsets += cumsum_subbin_sizes(output_bin_sizes);
  Variable filtered_input_bin_size = sum_subbin_sizes(output_bin_sizes);
  auto end = cumsum(filtered_input_bin_size);
  const auto total_size = end.values<scipp::index>().as_span().back();
  end = broadcast(end, data.dims()); // required for some cases of rebinning
  const auto filtered_input_bin_ranges =
      zip(end - filtered_input_bin_size, end);

  // Perform actual binning step for data, all coords, all masks, ...
  auto out_buffer = dataset::transform(bins_view<T>(data), [&](auto &&var) {
    if (!is_bins(var))
      return std::forward<decltype(var)>(var);
    const auto &[input_indices, buffer_dim, in_buffer] =
        var.template constituents<core::bin<VariableConstView>>();
    static_cast<void>(input_indices);
    auto out = resize_default_init(in_buffer, buffer_dim, total_size);
    std::cout << filtered_input_bin_ranges;
    std::cout << out;
    std::cout << offsets;
    transform_in_place(subspan_view(out, buffer_dim, filtered_input_bin_ranges),
                       offsets, as_subspan_view(var), as_subspan_view(indices),
                       core::element::bin);
    return out;
  });

  // Up until here the output was viewed with same bin index ranges as input.
  // Now switch to desired final bin indices.
  auto output_dims = merge(output_bin_sizes.dims(), dims);
  // auto bin_sizes =
  //    reshape(std::get<2>(output_bin_sizes.constituents<core::bin<Variable>>()),
  //            output_dims);
  auto bin_sizes = makeVariable<scipp::index>(
      output_dims, Values(flatten_subbin_sizes(output_bin_sizes)));
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
template <class Coords, class Masks, class Attrs>
DataArray add_metadata(std::tuple<DataArray, Variable> &&proto,
                       const Coords &coords, const Masks &masks,
                       const Attrs &attrs,
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
  auto out_coords = extract_unbinned(buffer, get_coords);
  for (const auto &c : {edges, groups})
    for (const auto &coord : c) {
      dims.emplace(coord.dims().inner());
      out_coords[coord.dims().inner()] = copy(coord);
    }
  for (const auto &[dim_, coord] : coords)
    if (!rebinned(coord))
      out_coords[dim_] = copy(coord);
  auto out_masks = extract_unbinned(buffer, get_masks);
  for (const auto &[name, mask] : masks)
    if (!rebinned(mask))
      out_masks[name] = copy(mask);
  auto out_attrs = extract_unbinned(buffer, get_attrs);
  for (const auto &[dim_, coord] : attrs)
    if (!rebinned(coord))
      out_attrs[dim_] = copy(coord);
  return {make_bins(zip(end - bin_sizes, end), buffer_dim, std::move(buffer)),
          std::move(out_coords), std::move(out_masks), std::move(out_attrs)};
}

class TargetBinBuilder {
  enum class AxisAction { Group, Bin, Existing, Join };

public:
  const Dimensions &dims() const noexcept { return m_dims; }

  /// `bin_coords` may optionally be used to provide bin-based coords, e.g., for
  /// data that has prior grouping but did not retain the original group coord
  /// for every event.
  template <class Coords, class BinCoords = CoordsConstView>
  void build(const VariableView &indices, Coords &&coords,
             BinCoords &&bin_coords = {}) {
    const auto get_coord = [&](const Dim dim) {
      return coords.count(dim) ? coords[dim] : Variable(bin_coords.at(dim));
    };
    offsets = makeVariable<scipp::index>(Values{0});
    nbin = dims().volume() * units::one;
    for (const auto &[action, dim, key] : m_actions) {
      if (action == AxisAction::Group)
        update_indices_by_grouping(indices, get_coord(dim), key);
      else if (action == AxisAction::Bin) {
        /*
        if (bin_coords.count(dim)) {
          printf("existing grouping along %s\n", to_string(dim).c_str());
          const auto &bin_coord = bin_coords.at(dim);
          // must include all rebinned dims... then carry through index updating
          // in parallel to event indices no, no need to broadcast, all inner
          // dims have same offsets
          auto bin_indices = makeVariable<scipp::index>(bin_coord.dims());
          update_indices_by_binning(bin_indices, bin_coord, key);
          // bin_indices contains index of bin coord value in new key
          // TODO split into begin and end, fix -1
          const auto begin =
              bin_indices.slice({dim, 0, bin_indices.dims()[dim] - 1});
          const auto end =
              bin_indices.slice({dim, 1, bin_indices.dims()[dim]}) +
              1 * units::one;
          const auto indices_ = zip(begin, end);
          const VariableConstView coord = bin_coord;
          // TODO use in update_indices_by_binning
          const auto coord_ = make_non_owning_bins(indices_, dim, coord);
          const auto inner_volume = dims().volume() / dims()[dim] * units::one;
          nbin = (end - begin) * inner_volume;
          offsets = begin * inner_volume;
        }
        */
        // for every input bin:
        // - candidate output begin bin
        // - candidate output end bin
        // => non-owning bins over coord..?
        // need nbin as variable? (different extent in every input bin)
        // build bin_sizes here instead?
        // - begin and end created here
        // - scale by length of all subsequent actions
        // - run bin_sizes (returning vector of size (end-begin))
        //   - does not need to handle offset, since indices start from 0 for
        //     every input bin
        // - create SubbinSizes with offset=begin
        if (coords.count(dim))
          update_indices_by_binning(indices, coords[dim], key);
        else {
          const auto &bin_coord = bin_coords.at(dim);
          auto bin_indices = makeVariable<scipp::index>(bin_coord.dims());
          update_indices_by_binning(bin_indices, bin_coord, key);
          indices *= bin_coord.dims().volume() * units::one;
          indices += bin_indices;
          // m_nbin /= key.dims().volume() - 1;
        }
      } else if (action == AxisAction::Existing)
        update_indices_from_existing(indices, dim);
      else if (action == AxisAction::Join) {
        ; // target bin 0 for all
      }
    }
  }

  auto edges() const noexcept {
    std::vector<VariableConstView> vars;
    for (const auto &[action, dim, key] : m_actions) {
      static_cast<void>(dim);
      if (action == AxisAction::Bin || action == AxisAction::Join)
        vars.emplace_back(key);
    }
    return vars;
  }

  auto groups() const noexcept {
    std::vector<VariableConstView> vars;
    for (const auto &[action, dim, key] : m_actions) {
      static_cast<void>(dim);
      if (action == AxisAction::Group)
        vars.emplace_back(key);
    }
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

  Variable offsets;
  Variable nbin;

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
template <class Coords>
auto axis_actions(const VariableConstView &data, const Coords &coords,
                  const std::vector<VariableConstView> &edges,
                  const std::vector<VariableConstView> &groups) {
  TargetBinBuilder builder;
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

class HideMasked {
public:
  template <class Masks>
  HideMasked(const VariableConstView &data, const Masks &masks,
             const Dimensions &dims) {
    const auto &[begin_end, buffer_dim, buffer] =
        data.constituents<core::bin<DataArray>>();
    auto [begin, end] = unzip(begin_end);
    for (const auto dim : dims.labels()) {
      auto mask = irreducible_mask(masks, dim);
      if (mask) {
        begin *= ~mask;
        end *= ~mask;
      }
    }
    m_indices = zip(begin, end);
    m_data = make_non_owning_bins(m_indices, buffer_dim, buffer);
  }
  VariableConstView operator()() const { return m_data; }

private:
  Variable m_indices; // keep alive indices
  Variable m_data;
};

template <class T> class TargetBins {
public:
  TargetBins(const VariableConstView &var, const Dimensions &dims) {
    // In some cases all events in an input bin map to the same output, but
    // right now bin<> cannot handle this and requires target bin indices for
    // every bin element.
    const auto &[begin_end, dim, buffer] = var.constituents<core::bin<T>>();
    m_target_bins_buffer = (dims.volume() > std::numeric_limits<int32_t>::max())
                               ? makeVariable<int64_t>(buffer.dims())
                               : makeVariable<int32_t>(buffer.dims());
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
  TargetBins<T> target_bins(var, builder.dims());

  builder.build(*target_bins, std::map<Dim, Variable>{});
  auto [buffer, bin_sizes] = bin<DataArray>(var, *target_bins, builder);
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
    if (array.coords().contains(dim)) {
      if (array.coords()[dim].dims().ndim() != 1 &&
          array.coords()[dim].dims().contains(reductionDim))
        builder.join(dim, array.coords()[dim]);
      else if (dim != reductionDim)
        builder.existing(dim, array.dims()[dim]);
    }

  HideMasked hide_masked(array.data(), array.masks(), builder.dims());
  const auto masked = hide_masked();
  TargetBins<DataArrayConstView> target_bins(masked, builder.dims());
  builder.build(*target_bins, array.coords());
  return add_metadata(bin<DataArrayConstView>(masked, *target_bins, builder),
                      array.coords(), array.masks(), array.attrs(),
                      builder.edges(), builder.groups(), {reductionDim});
}

namespace {
void validate_bin_args(const std::vector<VariableConstView> &edges,
                       const std::vector<VariableConstView> &groups) {
  if (edges.empty()) {
    if (groups.empty()) {
      throw except::BucketError(
          "Arguments 'edges' and 'groups' of scipp.bin are "
          "both empty. At least one must be set.");
    }
  } else {
    for (std::size_t i = 0; i < edges.size(); ++i) {
      if (const auto &shape = edges[i].dims().shape();
          std::any_of(shape.begin(), shape.end(),
                      [](const scipp::index s) { return s == 1; })) {
        throw except::BucketError("Not enough bin edges in dim " +
                                  std::to_string(i) + ". Need at least 2.");
      }
    }
  }
}
} // namespace

DataArray bin(const DataArrayConstView &array,
              const std::vector<VariableConstView> &edges,
              const std::vector<VariableConstView> &groups) {
  validate_bin_args(edges, groups);
  const auto &data = array.data();
  const auto &coords = array.coords();
  const auto &masks = array.masks();
  const auto &attrs = array.attrs();
  if (data.dtype() == dtype<core::bin<DataArray>>) {
    return bin(data, coords, masks, attrs, edges, groups);
  } else {
    // Pretend existing binning along outermost binning dim to enable threading
    const auto dim = data.dims().inner();
    const auto size = std::max(scipp::index(1), data.dims()[dim]);
    // TODO automatic setup with reasonable bin count
    const auto stride = std::max(scipp::index(1), size / 24);
    auto begin = make_range(0, size, stride,
                            groups.empty() ? edges.front().dims().inner()
                                           : groups.front().dims().inner());
    auto end = begin + stride * units::one;
    end.values<scipp::index>().as_span().back() = data.dims()[dim];
    const auto indices = zip(begin, end);
    const auto tmp = make_non_owning_bins(indices, dim, array);
    auto target_bins_buffer =
        (data.dims().volume() > std::numeric_limits<int32_t>::max())
            ? makeVariable<int64_t>(data.dims())
            : makeVariable<int32_t>(data.dims());
    auto builder = axis_actions(data, coords, edges, groups);
    builder.build(target_bins_buffer, coords);
    const auto target_bins = make_non_owning_bins(
        indices, dim, VariableConstView(target_bins_buffer));
    return add_metadata(bin<DataArrayConstView>(tmp, target_bins, builder),
                        coords, masks, attrs, builder.edges(), builder.groups(),
                        {});
  }
}

template <class Coords, class Masks, class Attrs>
DataArray bin(const VariableConstView &data, const Coords &coords,
              const Masks &masks, const Attrs &attrs,
              const std::vector<VariableConstView> &edges,
              const std::vector<VariableConstView> &groups) {
  auto builder = axis_actions(data, coords, edges, groups);
  HideMasked hide_masked(data, masks, builder.dims());
  const auto masked = hide_masked();
  TargetBins<DataArrayConstView> target_bins(masked, builder.dims());
  builder.build(*target_bins, bins_view<DataArrayConstView>(masked).coords(),
                coords);
  return add_metadata(bin<DataArrayConstView>(masked, *target_bins, builder),
                      coords, masks, attrs, builder.edges(), builder.groups(),
                      {});
}

template DataArray bin(const VariableConstView &,
                       const std::map<Dim, VariableConstView> &,
                       const std::map<std::string, VariableConstView> &,
                       const std::map<Dim, VariableConstView> &,
                       const std::vector<VariableConstView> &,
                       const std::vector<VariableConstView> &);

} // namespace scipp::dataset
