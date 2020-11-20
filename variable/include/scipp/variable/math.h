// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable abs(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable abs(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView abs(const VariableConstView &var,
                                       const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable dot(const VariableConstView &a,
                                                 const VariableConstView &b);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable norm(const VariableConstView &var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sqrt(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sqrt(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView sqrt(const VariableConstView &var,
                                        const VariableView &out);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
reciprocal(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable reciprocal(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView reciprocal(const VariableConstView &var,
                                              const VariableView &out);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable exp(const VariableConstView &var);
SCIPP_VARIABLE_EXPORT VariableView exp(const VariableConstView &var,
                                       const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable log(const VariableConstView &var);
SCIPP_VARIABLE_EXPORT VariableView log(const VariableConstView &var,
                                       const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
log10(const VariableConstView &var);

} // namespace scipp::variable
