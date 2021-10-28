// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

/// Implementation details of `bin`. Not for public use.
namespace scipp::variable::bin_detail {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable begin_edge(const Variable &coord,
                                                        const Variable &edges);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable end_edge(const Variable &coord,
                                                      const Variable &edges);

SCIPP_VARIABLE_EXPORT Variable
cumsum_exclusive_subbin_sizes(const Variable &var);
SCIPP_VARIABLE_EXPORT Variable sum_subbin_sizes(const Variable &var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::vector<scipp::index>
flatten_subbin_sizes(const Variable &var, const scipp::index length);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
subbin_sizes_cumsum_exclusive(const Variable &var, const Dim dim);
SCIPP_VARIABLE_EXPORT void subbin_sizes_add_intersection(Variable &a,
                                                         const Variable &b);

} // namespace scipp::variable::bin_detail
