// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable operator|(const VariableConstView &a,
                                         const VariableConstView &b);
SCIPP_VARIABLE_EXPORT Variable operator&(const VariableConstView &a,
                                         const VariableConstView &b);
SCIPP_VARIABLE_EXPORT Variable operator^(const VariableConstView &a,
                                         const VariableConstView &b);

} // namespace scipp::variable
