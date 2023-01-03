// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/bin.h"
#include "scipp/core/element/map_to_bins.h"

#include "scipp/variable/cumulative.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

#include "bin_detail.h"

namespace scipp::dataset::bin_detail {

/// Implementation detail of dataset::bin
void map_to_bins(Variable &out, const Variable &var, const Variable &offsets,
                 const Variable &indices) {
  transform_in_place(out, offsets, var, indices, core::element::bin, "bin");
}

Variable make_range(const scipp::index begin, const scipp::index end,
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

namespace {
template <class Index>
Variable groups_to_map(const Variable &var, const Dim dim) {
  return variable::transform(subspan_view(var, dim),
                             core::element::groups_to_map<Index>,
                             "scipp.bin.groups_to_map");
}
} // namespace

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

} // namespace scipp::dataset::bin_detail
