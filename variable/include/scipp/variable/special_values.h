// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
isnan(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
isinf(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
isfinite(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
isposinf(const VariableConstView &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
isneginf(const VariableConstView &var);

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

} // namespace scipp::variable
