// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
#define SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H

#include "scipp/core/dataset.h"

namespace scipp::core {

// Helpers for in-place reductions and reductions with groupby.
void flatten_impl(const VariableProxy &summed, const VariableConstProxy &var,
                  const VariableConstProxy &mask);
void sum_impl(const VariableProxy &summed, const VariableConstProxy &var);
void all_impl(const VariableProxy &out, const VariableConstProxy &var);
void any_impl(const VariableProxy &out, const VariableConstProxy &var);
void max_impl(const VariableProxy &out, const VariableConstProxy &var);
void min_impl(const VariableProxy &out, const VariableConstProxy &var);

// Helpers for reductions for DataArray and Dataset, which include masks.
[[nodiscard]] Variable mean(const VariableConstProxy &var, const Dim dim,
                            const MasksConstProxy &masks);
VariableProxy mean(const VariableConstProxy &var, const Dim dim,
                   const MasksConstProxy &masks, const VariableProxy &out);
[[nodiscard]] Variable flatten(const VariableConstProxy &var, const Dim dim,
                               const MasksConstProxy &masks);
[[nodiscard]] Variable sum(const VariableConstProxy &var, const Dim dim,
                           const MasksConstProxy &masks);
VariableProxy sum(const VariableConstProxy &var, const Dim dim,
                  const MasksConstProxy &masks, const VariableProxy &out);

} // namespace scipp::core

#endif // SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
