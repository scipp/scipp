// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT VariableConstView
left_edge(const VariableConstView &edges);
[[nodiscard]] SCIPP_VARIABLE_EXPORT VariableConstView
right_edge(const VariableConstView &edges);

// TODO These are implementation details of `bin`. namespace bin_detail {?
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
begin_edge(const VariableConstView &coord, const VariableConstView &edges);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
end_edge(const VariableConstView &coord, const VariableConstView &edges);

SCIPP_VARIABLE_EXPORT Variable
cumsum_exclusive_subbin_sizes(const VariableConstView &var);
SCIPP_VARIABLE_EXPORT Variable sum_subbin_sizes(const VariableConstView &var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::vector<scipp::index>
flatten_subbin_sizes(const VariableConstView &var, const scipp::index length);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
subbin_sizes_cumsum_exclusive(const VariableConstView &var, const Dim dim);
SCIPP_VARIABLE_EXPORT void
subbin_sizes_add_intersection(const VariableView &a,
                              const VariableConstView &b);

} // namespace scipp::variable
