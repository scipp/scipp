// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/bin_detail.h"
#include "scipp/core/element/util.h" // fill_zeros
#include "scipp/core/subbin_sizes.h"

#include "scipp/variable/accumulate.h"
#include "scipp/variable/bin_detail.h"
#include "scipp/variable/bin_util.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

namespace scipp::variable::bin_detail {

/// Index of the bin (given by `edges`) containing a coord value.
///
/// 0 if the coord is less than the first edge, nbin-1 if greater or equal last
/// edge. Assumes both `edges` and `coord` are sorted.
Variable begin_edge(const Variable &coord, const Variable &edges) {
  auto indices = makeVariable<scipp::index>(coord.dims(), sc_units::none);
  const auto dim = edges.dims().inner();
  if (indices.dims()[dim] == 0)
    return indices;
  auto bin = copy(indices.slice({dim, 0}));
  accumulate_in_place(bin, indices, coord, subspan_view(edges, dim),
                      core::element::begin_edge, "scipp.bin.begin_edge");
  return indices;
}

/// End bin
///
/// 1 if the coord is
/// nbin if the coord is greater than the last edge. Assumes both `edges` and
/// `coord` are sorted.
Variable end_edge(const Variable &coord, const Variable &edges) {
  auto indices = makeVariable<scipp::index>(coord.dims(), sc_units::none);
  const auto dim = edges.dims().inner();
  if (indices.dims()[dim] == 0)
    return indices;
  auto bin = copy(indices.slice({dim, 0}));
  accumulate_in_place(bin, indices, coord, subspan_view(edges, dim),
                      core::element::end_edge, "scipp.bin.end_edge");
  return indices;
}

Variable cumsum_exclusive_subbin_sizes(const Variable &var) {
  return transform<core::SubbinSizes>(
      var,
      overloaded{[](const sc_units::Unit &u) { return u; },
                 [](const auto &sizes) { return sizes.cumsum_exclusive(); }},
      "scipp.bin.cumsum_exclusive");
}

Variable sum_subbin_sizes(const Variable &var) {
  return transform<core::SubbinSizes>(
      var,
      overloaded{[](const sc_units::Unit &u) { return u; },
                 [](const auto &sizes) { return sizes.sum(); }},
      "scipp.bin.sum_subbin_sizes");
}

Variable subbin_sizes_cumsum_exclusive(const Variable &var, const Dim dim) {
  if (var.dims()[dim] == 0)
    return copy(var);
  auto cumulative = copy(var.slice({dim, 0}));
  fill_zeros(cumulative);
  auto out = copy(var);
  accumulate_in_place(cumulative, out,
                      core::element::subbin_sizes_exclusive_scan,
                      "scipp.bin.subbin_sizes_cumsum_exclusive");
  return out;
}

void subbin_sizes_add_intersection(Variable &a, const Variable &b) {
  transform_in_place(a, b, core::element::subbin_sizes_add_intersection,
                     "scipp.bin.subbin_sizes_add_intersection");
}

} // namespace scipp::variable::bin_detail
