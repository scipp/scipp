// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/generated_trigonometry.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sin(const Variable &var);
SCIPP_VARIABLE_EXPORT Variable &sin(const Variable &var, Variable &out);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable cos(const Variable &var);
SCIPP_VARIABLE_EXPORT Variable &cos(const Variable &var, Variable &out);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable tan(const Variable &var);
SCIPP_VARIABLE_EXPORT Variable &tan(const Variable &var, Variable &out);

} // namespace scipp::variable
