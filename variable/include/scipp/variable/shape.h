// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable broadcast(const VariableConstView &var,
                                         const Dimensions &dims);
SCIPP_VARIABLE_EXPORT Variable concatenate(const VariableConstView &a1,
                                           const VariableConstView &a2,
                                           const Dim dim);
SCIPP_VARIABLE_EXPORT Variable
permute(const Variable &var, const Dim dim,
        const std::vector<scipp::index> &indices);
SCIPP_VARIABLE_EXPORT Variable resize(const VariableConstView &var,
                                      const Dim dim, const scipp::index size);
SCIPP_VARIABLE_EXPORT Variable reverse(Variable var, const Dim dim);

} // namespace scipp::variable
