// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/core/flags.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable mean(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable mean(const Variable &var,
                                                  const Dim dim);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sum(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sum(const Variable &var,
                                                 const Dim dim);

// Logical reductions
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable any(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable any(const Variable &var,
                                                 const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable all(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable all(const Variable &var,
                                                 const Dim dim);

// Other reductions
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable max(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable max(const Variable &var,
                                                 const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable min(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable min(const Variable &var,
                                                 const Dim dim);
// Reduction operations ignoring or zeroing nans
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nanmax(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nanmax(const Variable &var,
                                                    const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nanmin(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nanmin(const Variable &var,
                                                    const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nansum(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nansum(const Variable &var,
                                                    const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nanmean(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable nanmean(const Variable &var,
                                                     const Dim dim);

// Reductions of all events within a bin.
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_sum(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_nansum(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_max(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_nanmax(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_min(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_nanmin(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_all(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_any(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_mean(const Variable &data);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bins_nanmean(const Variable &data);

// These reductions accumulate their results in their first argument
// without erasing its current contents.
SCIPP_VARIABLE_EXPORT void sum_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void nansum_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void all_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void any_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void max_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void nanmax_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void min_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void nanmin_into(Variable &accum, const Variable &var);
} // namespace scipp::variable
