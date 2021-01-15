// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/bin_util.h"
#include "scipp/core/element/util.h" // fill_zeros
#include "scipp/core/subbin_sizes.h"

#include "scipp/variable/bin_util.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

/// Return view onto left bin edges, i.e., all but the last edge
VariableConstView left_edge(const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  return edges.slice({dim, 0, edges.dims()[dim] - 1});
}

/// Return view onto right bin edges, i.e., all but the first edge
VariableConstView right_edge(const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  return edges.slice({dim, 1, edges.dims()[dim]});
}

/// Index of the bin (given by `edges`) containing a coord value.
///
/// 0 if the coord is less than the first edge, nbin-1 if greater or equal last
/// edge. Assumes both `edges` and `coord` are sorted.
Variable begin_edge(const VariableConstView &coord,
                    const VariableConstView &edges) {
  auto indices = makeVariable<scipp::index>(coord.dims());
  const auto dim = edges.dims().inner();
  Variable bin(indices.slice({dim, 0}));
  accumulate_in_place(bin, indices, coord, subspan_view(edges, dim),
                      core::element::begin_bin);
  return indices;
}

/// End bin
///
/// 1 if the coord is
/// nbin if the coord is greater than the last edge. Assumes both `edges` and
/// `coord` are sorted.
Variable end_edge(const VariableConstView &coord,
                  const VariableConstView &edges) {
  auto indices = makeVariable<scipp::index>(coord.dims());
  const auto dim = edges.dims().inner();
  Variable bin(indices.slice({dim, 0}));
  accumulate_in_place(bin, indices, coord, subspan_view(edges, dim),
                      core::element::end_bin);
  return indices;
}

// 1. subbin_sizes counts event for every target bin of every input bin
//    its output is input centric
// 2. reshape buffer
// 3. non-owning bins (output-centric based on output bin ranges computed from
//    begin_end and end_bin)
// 4. cumsum_bins...but would need to transpose? should *first* iterate inputs,
//    then bins
//
// are dims inside ragged rebinned dim guaranteed to have some begin_bin and
// end_bin? if not then above does not work, I think
// reshape(output_bin_sizes_buffer,
//        Dimensions({Dim("subbin"), Dim("bin")}, {nsubbin, nbin}));
// make_non_owning_bins(indices(dstdims), Dim("subbin"), ..);
//
// subbin_sizes looks like this:
//
// ---1
// --11
// --4-
// 111-
// 2---
//
// each row corresponds to an input bin
// each column corresponds to an input bin
// the example is for a single rebinned dim
// `-` is 0
//
// We need to store this an a packed format to avoid O(M*N) memory (and compute)
// requirements.
// The difficulty is that we need to apply sum and cumsum along both axes.
// Can a helper class help?

// TODO next steps:
// - implement operations needed for setup in bin.cpp:bin<T>
//   - sum, cumsum
//   - cumsum_bins (cumsum along sizes in SubbinSizes)
//   - buckets::sum (sum along sizes in SubbinSizes)
// - refactor bin_sizes so it creates Variable<SubbinSizes> instead of binned
//   variable -> just return vector (and begin?)?
// - compute begin and end (and nbin) in TargetBinBuilder::build, forward those
//   to bin<T>
// - use computed begin and end in `update_indices_by*`:
//   - create subspan views so each input bin in the transform sees only
//     relevant section of output bin edges (or groups)

/// Fill variable with zeros.
void subbin_sizes_fill_zeros(const VariableView &var) {
  transform_in_place<core::SubbinSizes>(var, core::element::fill_zeros);
}

Variable cumsum_subbin_sizes(const VariableConstView &var) {
  return transform<core::SubbinSizes>(
      var, overloaded{[](const units::Unit &u) { return u; },
                      [](const auto &sizes) { return sizes.cumsum(); }});
}

Variable sum_subbin_sizes(const VariableConstView &var) {
  return transform<core::SubbinSizes>(
      var, overloaded{[](const units::Unit &u) { return u; },
                      [](const auto &sizes) { return sizes.sum(); }});
}

std::vector<scipp::index> flatten_subbin_sizes(const VariableConstView &var,
                                               const scipp::index length) {
  std::vector<scipp::index> flat;
  for (const auto &val : var.values<core::SubbinSizes>()) {
    flat.insert(flat.end(), val.sizes().begin(), val.sizes().end());
    for (scipp::index i = 0; i < length - scipp::size(val.sizes()); ++i)
      flat.push_back(0);
  }
  return flat;
}

Variable subbin_sizes_exclusive_scan(const VariableConstView &var,
                                     const Dim dim) {
  if (var.dims()[dim] == 0)
    return Variable{var};
  Variable cumulative(var.slice({dim, 0}));
  subbin_sizes_fill_zeros(cumulative);
  Variable out(var);
  accumulate_in_place(cumulative, out,
                      core::element::subbin_sizes_exclusive_scan);
  return out;
}

void subbin_sizes_add_intersection(const VariableView &a,
                                   const VariableConstView &b) {
  transform_in_place(a, b, core::element::subbin_sizes_add_intersection);
}

} // namespace scipp::variable
