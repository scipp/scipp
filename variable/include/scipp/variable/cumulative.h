// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable cumsum(
    const VariableConstView &var, const Dim dim, const bool inclusive = true);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum(const VariableConstView &var, const bool inclusive = true);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum_bins(const VariableConstView &var, const bool inclusive = true);

} // namespace scipp::variable
