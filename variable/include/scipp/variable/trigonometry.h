// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sin(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sin(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView sin(const VariableConstView &var,
                                       const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable cos(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable cos(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView cos(const VariableConstView &var,
                                       const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable tan(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable tan(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView tan(const VariableConstView &var,
                                       const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable asin(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable asin(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView asin(const VariableConstView &var,
                                        const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable acos(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable acos(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView acos(const VariableConstView &var,
                                        const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable atan(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable atan(Variable &&var);
SCIPP_VARIABLE_EXPORT VariableView atan(const VariableConstView &var,
                                        const VariableView &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable atan2(const VariableConstView &y,
                                                   const VariableConstView &x);
SCIPP_VARIABLE_EXPORT VariableView atan2(const VariableConstView &y,
                                         const VariableConstView &x,
                                         const VariableView &out);

} // namespace scipp::variable
