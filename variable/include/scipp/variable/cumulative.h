// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
exclusive_scan(const VariableConstView &var, const Dim dim);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
exclusive_scan_bins(const VariableConstView &var);

} // namespace scipp::variable
