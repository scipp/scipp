// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <scipp/variable/variable.h>

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable subspan_view(Variable &var, const Dim dim);
SCIPP_VARIABLE_EXPORT Variable subspan_view(const VariableView &var,
                                            const Dim dim);
SCIPP_VARIABLE_EXPORT Variable subspan_view(const VariableConstView &var,
                                            const Dim dim);

} // namespace scipp::variable
