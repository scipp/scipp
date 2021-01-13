// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable begin_bin_sorted(
    const VariableConstView &coord, const VariableConstView &edges);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
end_bin_sorted(const VariableConstView &coord, const VariableConstView &edges);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Variable, Variable>
subbin_offsets(const VariableConstView &start_, const VariableConstView &stop_,
               const VariableConstView &subbin_sizes_, const scipp::index nsrc,
               const scipp::index ndst, const scipp::index nbin);

SCIPP_VARIABLE_EXPORT void subbin_sizes_fill_zeros(const VariableView &var);
SCIPP_VARIABLE_EXPORT Variable
cumsum_subbin_sizes(const VariableConstView &var);
SCIPP_VARIABLE_EXPORT Variable sum_subbin_sizes(const VariableConstView &var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::vector<scipp::index>
flatten_subbin_sizes(const VariableConstView &var, const scipp::index length);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
subbin_sizes_exclusive_scan(const VariableConstView &var, const Dim dim);
SCIPP_VARIABLE_EXPORT void
subbin_sizes_add_intersection(const VariableView &a,
                              const VariableConstView &b);

} // namespace scipp::variable
