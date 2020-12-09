// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable mean(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable mean(const VariableConstView &var,
                                                  const Dim dim);
SCIPP_VARIABLE_EXPORT VariableView mean(const VariableConstView &var,
                                        const Dim dim, const VariableView &out);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sum(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sum(const VariableConstView &var,
                                                 const Dim dim);
SCIPP_VARIABLE_EXPORT VariableView sum(const VariableConstView &var,
                                       const Dim dim, const VariableView &out);

// Logical reductions
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable any(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable any(const VariableConstView &var,
                                                 const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable all(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable all(const VariableConstView &var,
                                                 const Dim dim);

// Other reductions
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable max(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable max(const VariableConstView &var,
                                                 const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable min(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable min(const VariableConstView &var,
                                                 const Dim dim);
// Reduction operations ignoring or zeroing nans
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nanmax(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nanmax(const VariableConstView &var, const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nanmin(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nanmin(const VariableConstView &var, const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nansum(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nansum(const VariableConstView &var, const Dim dim);
SCIPP_VARIABLE_EXPORT VariableView nansum(const VariableConstView &var,
                                          const Dim dim,
                                          const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nanmean(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nanmean(const VariableConstView &var, const Dim dim);
SCIPP_VARIABLE_EXPORT VariableView nanmean(const VariableConstView &var,
                                           const Dim dim,
                                           const VariableView &out);

} // namespace scipp::variable
