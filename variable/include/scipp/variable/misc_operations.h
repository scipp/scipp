// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
SCIPP_VARIABLE_EXPORT Variable rebin(const VariableConstView &var,
                                     const Dim dim,
                                     const VariableConstView &oldCoord,
                                     const VariableConstView &newCoord);

SCIPP_VARIABLE_EXPORT VariableView
nan_to_num(const VariableConstView &var, const VariableConstView &replacement,
           const VariableView &out);
SCIPP_VARIABLE_EXPORT VariableView positive_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement,
    const VariableView &out);
SCIPP_VARIABLE_EXPORT VariableView negative_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement,
    const VariableView &out);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
nan_to_num(const VariableConstView &var, const VariableConstView &replacement);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable pos_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable neg_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement);
namespace geometry {
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
position(const VariableConstView &x, const VariableConstView &y,
         const VariableConstView &z);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable x(const VariableConstView &pos);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable y(const VariableConstView &pos);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable z(const VariableConstView &pos);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
rotate(const VariableConstView &pos, const VariableConstView &rot);
SCIPP_VARIABLE_EXPORT VariableView rotate(const VariableConstView &pos,
                                          const VariableConstView &rot,
                                          const VariableView &out);

} // namespace geometry

} // namespace scipp::variable
