// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

#include "scipp/variable/generated_math.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable abs(Variable &&var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable dot(const VariableConstView &a,
                                                 const VariableConstView &b);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable norm(const VariableConstView &var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sqrt(Variable &&var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable reciprocal(Variable &&var);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
floor_div(const VariableConstView &a, const VariableConstView &b);

} // namespace scipp::variable
