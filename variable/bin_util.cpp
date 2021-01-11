// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/bin_util.h"

#include "scipp/variable/bin_util.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

// TODO two cases, input edges or groups
Variable bin_index_sorted(const VariableConstView &coord,
                          const VariableConstView &edges) {
  const auto dim = edges.dims().inner();
  // TODO this is M * N, use linear alg instead
  return transform(coord, subspan_view(edges, dim), core::element::first);
}

// start: first (outer) dst bin overlapping input bin
// stop: first (outer) dst bin not overlapping input bin
std::tuple<Variable, Variable>
subbin_offsets(const VariableConstView &start_, const VariableConstView &stop_,
               const VariableConstView &subbin_sizes_, const scipp::index nsrc,
               const scipp::index ndst, const scipp::index nbin) {
  // TODO This will not work if we have untouched outer dims
  // => transform with subspans for untouched outer dims, use this as kernel...
  // but we have multiple returns
  Variable cumsum_(subbin_sizes_);
  auto output_bin_sizes_ =
      makeVariable<scipp::index>(Dims{Dim("dst")}, Shape{ndst});
  const auto start = start_.values<scipp::index>();
  const auto stop = stop_.values<scipp::index>();
  const auto subbin_sizes = subbin_sizes_.values<scipp::index>();
  const auto cumsum = cumsum_.values<scipp::index>();
  const auto output_bin_sizes = output_bin_sizes_.values<scipp::index>();
  scipp::index sum = 0;
  scipp::index dst = 0;
  scipp::index src = 0;
  while (start[src] > dst)
    ++dst;
  for (; dst < ndst; ++dst) {
    scipp::index src0 = src;
    while ((src < nsrc) && (start[src] <= dst) && (dst < stop[src]))
      ++src;
    for (scipp::index bin = 0; bin < nbin; ++bin) {
      for (scipp::index s = src0; s < src; ++s) {
        const auto subbin_size = subbin_sizes[(s + dst) * nbin + bin];
        sum += subbin_size;
        cumsum[(s + dst) * nbin + bin] = sum;
        output_bin_sizes[dst] += subbin_size;
      }
    }
    --src;
  }
  return {cumsum_, output_bin_sizes_};
}

/// - `subbins` is a binned variable with sub-bin indices, i.e., new bins within
///   bins
/// - `begin_bin` is the first output bin overlapping an input bin
/// - `end_bin` is the output bin after the last overlapping an input bin
/*
Variable subbin_sizes(const VariableConstView &subbins,
                      const VariableConstView &begin_bin,
                      const VariableConstView &end_bin) {
  const auto nbin = end_bin - begin_bin;
  const auto end = cumsum(nbin);
  const auto dim = variable::variableFactory().elem_dim(subbins);
  auto sizes = make_bins(
      zip(end - nbin, end), dim,
      makeVariable<scipp::index>(
          Dims{dim}, Shape{end.values<scipp::index>().as_span().back()}));
  variable::transform_in_place(
      as_subspan_view(sizes), as_subspan_view(subbins),
      core::element::count_indices); // transform bins, not bin element
  return sizes;
}
*/

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
SubbinSizes::SubbinSizes(const scipp::index offset,
                         std::vector<scipp::index> &&sizes)
    : m_offset(offset), m_sizes(std::move(sizes)) {}

bool operator==(const SubbinSizes &a, const SubbinSizes &b) {
  return std::tie(a.offset(), a.sizes()) == std::tie(b.offset(), b.sizes());
}

// is this good enough for what we need? for cumsum we want to avoid filling in
// the area of zero padding.. instead (inclusive_scan, similar for
// exclusive_scan):
//
// sum = intersection(sum, x) # drop lower subbins
// sum += x
// x = sum
SubbinSizes operator+(const SubbinSizes &a, const SubbinSizes &b) {
  const auto begin = std::min(a.offset(), b.offset());
  const auto end =
      std::max(a.offset() + a.sizes().size(), b.offset() + b.sizes().size());
  std::vector<scipp::index> sizes(end - begin);
  scipp::index current = a.offset() - begin;
  for (const auto &size : a.sizes())
    sizes[current++] += size;
  current = b.offset() - begin;
  for (const auto &size : b.sizes())
    sizes[current++] += size;
  return {begin, std::move(sizes)};
}

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

} // namespace scipp::variable
