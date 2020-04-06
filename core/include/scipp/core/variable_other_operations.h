// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/variable.h"

namespace scipp::core {

SCIPP_CORE_EXPORT Variable astype(const VariableConstView &var,
                                  const DType type);

[[nodiscard]] SCIPP_CORE_EXPORT Variable
reciprocal(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable reciprocal(Variable &&var);
SCIPP_CORE_EXPORT VariableView reciprocal(const VariableConstView &var,
                                          const VariableView &out);

SCIPP_CORE_EXPORT std::vector<Variable>
split(const Variable &var, const Dim dim,
      const std::vector<scipp::index> &indices);
[[nodiscard]] SCIPP_CORE_EXPORT Variable abs(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable abs(Variable &&var);
SCIPP_CORE_EXPORT VariableView abs(const VariableConstView &var,
                                   const VariableView &out);
SCIPP_CORE_EXPORT Variable broadcast(const VariableConstView &var,
                                     const Dimensions &dims);
SCIPP_CORE_EXPORT Variable concatenate(const VariableConstView &a1,
                                       const VariableConstView &a2,
                                       const Dim dim);
SCIPP_CORE_EXPORT Variable dot(const Variable &a, const Variable &b);
SCIPP_CORE_EXPORT Variable filter(const Variable &var, const Variable &filter);
SCIPP_CORE_EXPORT Variable norm(const VariableConstView &var);
SCIPP_CORE_EXPORT Variable permute(const Variable &var, const Dim dim,
                                   const std::vector<scipp::index> &indices);
SCIPP_CORE_EXPORT Variable rebin(const VariableConstView &var, const Dim dim,
                                 const VariableConstView &oldCoord,
                                 const VariableConstView &newCoord);
SCIPP_CORE_EXPORT Variable resize(const VariableConstView &var, const Dim dim,
                                  const scipp::index size);
SCIPP_CORE_EXPORT Variable reverse(Variable var, const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable sqrt(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable sqrt(Variable &&var);
SCIPP_CORE_EXPORT VariableView sqrt(const VariableConstView &var,
                                    const VariableView &out);

SCIPP_CORE_EXPORT Variable copy(const VariableConstView &var);

SCIPP_CORE_EXPORT VariableView nan_to_num(const VariableConstView &var,
                                          const VariableConstView &replacement,
                                          const VariableView &out);
SCIPP_CORE_EXPORT VariableView positive_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement,
    const VariableView &out);
SCIPP_CORE_EXPORT VariableView negative_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement,
    const VariableView &out);

[[nodiscard]] SCIPP_CORE_EXPORT Variable
nan_to_num(const VariableConstView &var, const VariableConstView &replacement);
[[nodiscard]] SCIPP_CORE_EXPORT Variable pos_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement);
[[nodiscard]] SCIPP_CORE_EXPORT Variable neg_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement);
namespace geometry {
[[nodiscard]] SCIPP_CORE_EXPORT Variable position(const VariableConstView &x,
                                                  const VariableConstView &y,
                                                  const VariableConstView &z);
[[nodiscard]] SCIPP_CORE_EXPORT Variable x(const VariableConstView &pos);
[[nodiscard]] SCIPP_CORE_EXPORT Variable y(const VariableConstView &pos);
[[nodiscard]] SCIPP_CORE_EXPORT Variable z(const VariableConstView &pos);

} // namespace geometry

} // namespace scipp::core
