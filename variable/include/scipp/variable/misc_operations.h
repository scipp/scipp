// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable astype(const VariableConstView &var,
                                      const DType type);

SCIPP_VARIABLE_EXPORT std::vector<Variable>
split(const Variable &var, const Dim dim,
      const std::vector<scipp::index> &indices);
SCIPP_VARIABLE_EXPORT Variable filter(const Variable &var,
                                      const Variable &filter);

namespace geometry {
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
position(const VariableConstView &x, const VariableConstView &y,
         const VariableConstView &z);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable x(const VariableConstView &pos);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable y(const VariableConstView &pos);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable z(const VariableConstView &pos);

} // namespace geometry

} // namespace scipp::variable
