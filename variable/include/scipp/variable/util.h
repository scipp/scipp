// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable linspace(const VariableConstView &start,
                                        const VariableConstView &stop,
                                        const Dim dim, const scipp::index num);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable values(const VariableConstView &x);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
variances(const VariableConstView &x);

} // namespace scipp::variable
