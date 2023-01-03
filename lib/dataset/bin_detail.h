// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-dataset_export.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset::bin_detail {

template <class T> Variable as_subspan_view(T &&binned) {
  auto &&[indices, dim, buffer] = binned.template constituents<Variable>();
  if constexpr (std::is_const_v<std::remove_reference_t<T>>)
    return subspan_view(std::as_const(buffer), dim, indices);
  else
    return subspan_view(buffer, dim, indices);
}

void map_to_bins(Variable &out, const Variable &var, const Variable &offsets,
                 const Variable &indices);

Variable make_range(const scipp::index begin, const scipp::index end,
                    const scipp::index stride, const Dim dim);

void update_indices_by_binning(Variable &indices, const Variable &key,
                               const Variable &edges, const bool linspace);
void update_indices_by_grouping(Variable &indices, const Variable &key,
                                const Variable &groups);
void update_indices_from_existing(Variable &indices, const Dim dim);
Variable bin_sizes(const Variable &sub_bin, const Variable &offset,
                   const Variable &nbin);

} // namespace scipp::dataset::bin_detail
