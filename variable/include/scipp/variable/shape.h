// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
SCIPP_VARIABLE_EXPORT Variable resize(const VariableConstView &var,
                                      const VariableConstView &shape);
SCIPP_VARIABLE_EXPORT Variable reverse(Variable var, const Dim dim);
SCIPP_VARIABLE_EXPORT VariableView reshape(Variable &var,
                                           const Dimensions &dims);
SCIPP_VARIABLE_EXPORT Variable reshape(Variable &&var, const Dimensions &dims);
SCIPP_VARIABLE_EXPORT Variable reshape(const VariableConstView &view,
                                       const Dimensions &dims);
SCIPP_VARIABLE_EXPORT Variable fold(const VariableConstView &view,
                                    const Dim from_dim,
                                    const Dimensions &to_dims);
SCIPP_VARIABLE_EXPORT Variable flatten(const VariableConstView &view,
                                       const std::vector<Dim> &from_labels,
                                       const Dim to_dim);
SCIPP_VARIABLE_EXPORT VariableView transpose(Variable &var,
                                             const std::vector<Dim> &dims = {});
SCIPP_VARIABLE_EXPORT Variable transpose(Variable &&var,
                                         const std::vector<Dim> &dims = {});
SCIPP_VARIABLE_EXPORT VariableConstView
transpose(const VariableConstView &view, const std::vector<Dim> &dims = {});
SCIPP_VARIABLE_EXPORT VariableView transpose(const VariableView &view,
                                             const std::vector<Dim> &dims = {});

SCIPP_VARIABLE_EXPORT void squeeze(Variable &var, const std::vector<Dim> &dims);

SCIPP_VARIABLE_EXPORT void expect_same_volume(const Dimensions &old_dims,
                                              const Dimensions &new_dims);

SCIPP_VARIABLE_EXPORT void validate_fold_dims(const Dimensions &old_dims,
                                              const Dim from_dim,
                                              const Dimensions &to_dims);
SCIPP_VARIABLE_EXPORT
void validate_flatten_dims(const Dimensions &old_dims,
                           const Dimensions &from_dims, const Dim to_dim);
SCIPP_VARIABLE_EXPORT Dimensions fold_dims(const Dimensions &old_dims,
                                           const Dim from_dim,
                                           const Dimensions &to_dims);
SCIPP_VARIABLE_EXPORT Dimensions flatten_dims(const Dimensions &old_dims,
                                              const Dimensions &from_dims,
                                              const Dim to_dim);

} // namespace scipp::variable
