// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
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

// These reductions accumulate their results in their first argument
// without erasing its current contents.
SCIPP_VARIABLE_EXPORT void sum_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void all_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void any_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void max_into(Variable &accum, const Variable &var);
SCIPP_VARIABLE_EXPORT void min_into(Variable &accum, const Variable &var);

} // namespace scipp::variable
