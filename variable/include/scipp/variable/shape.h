// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
broadcast(const VariableConstView &var, const Dimensions &dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable concatenate(
    const VariableConstView &a1, const VariableConstView &a2, const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
permute(const Variable &var, const Dim dim,
        const std::vector<scipp::index> &indices);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
resize(const VariableConstView &var, const Dim dim, const scipp::index size);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
resize(const VariableConstView &var, const VariableConstView &shape);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable reverse(const Variable &var,
                                                     const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable reshape(const Variable &var,
                                                     const Dimensions &dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
reshape(const VariableConstView &view, const Dimensions &dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable fold(const VariableConstView &view,
                                                  const Dim from_dim,
                                                  const Dimensions &to_dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
flatten(const VariableConstView &view,
        const scipp::span<const Dim> &from_labels, const Dim to_dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
transpose(const Variable &var, const std::vector<Dim> &dims = {});

SCIPP_VARIABLE_EXPORT void squeeze(Variable &var, const std::vector<Dim> &dims);

SCIPP_VARIABLE_EXPORT void expect_same_volume(const Dimensions &old_dims,
                                              const Dimensions &new_dims);

} // namespace scipp::variable
