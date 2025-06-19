// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
concat(const std::span<const Variable> vars, const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
resize(const Variable &var, const Dim dim, const scipp::index size,
       const FillValue fill = FillValue::Default);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable resize(const Variable &var,
                                                    const Variable &shape);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable fold(const Variable &view,
                                                  const Dim from_dim,
                                                  const Dimensions &to_dims);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
flatten(const Variable &view, const std::span<const Dim> &from_labels,
        const Dim to_dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
transpose(const Variable &var, std::span<const Dim> dims = {});

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::vector<scipp::Dim>
dims_for_squeezing(const core::Sizes &data_dims,
                   std::optional<std::span<const Dim>> selected_dims);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
squeeze(const Variable &var, std::optional<std::span<const Dim>> dims);

} // namespace scipp::variable
