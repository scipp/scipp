// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sin(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sin(Variable &&var);
SCIPP_VARIABLE_EXPORT Variable &sin(const Variable &var, Variable &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable cos(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable cos(Variable &&var);
SCIPP_VARIABLE_EXPORT Variable &cos(const Variable &var, Variable &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable tan(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable tan(Variable &&var);
SCIPP_VARIABLE_EXPORT Variable &tan(const Variable &var, Variable &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable asin(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable asin(Variable &&var);
SCIPP_VARIABLE_EXPORT Variable &asin(const Variable &var, Variable &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable acos(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable acos(Variable &&var);
SCIPP_VARIABLE_EXPORT Variable &acos(const Variable &var, Variable &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable atan(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable atan(Variable &&var);
SCIPP_VARIABLE_EXPORT Variable &atan(const Variable &var, Variable &out);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable atan2(const Variable &y,
                                                   const Variable &x);
SCIPP_VARIABLE_EXPORT Variable &atan2(const Variable &y, const Variable &x,
                                      Variable &out);

} // namespace scipp::variable
