// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/flags.h"

#include "scipp-variable_export.h"
#include "scipp/variable/creation.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable broadcast(const Variable &var,
                                                       const Dimensions &dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
concat(const scipp::span<const Variable> vars, const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
resize(const Variable &var, const Dim dim, const scipp::index size,
       const FillValue fill = FillValue::Default);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable resize(const Variable &var,
                                                    const Variable &shape);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable fold(const Variable &view,
                                                  const Dim from_dim,
                                                  const Dimensions &to_dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
flatten(const Variable &view, const scipp::span<const Dim> &from_labels,
        const Dim to_dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
transpose(const Variable &var, scipp::span<const Dim> dims = {});

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
squeeze(const Variable &var, const std::vector<Dim> &dims);

} // namespace scipp::variable
