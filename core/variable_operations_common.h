// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
#define SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H

#include "scipp/core/variable.h"

namespace scipp::core {

// Helpers for in-place reductions and reductions with groupby.
void flatten_impl(const VariableProxy &summed, const VariableConstProxy &var,
                  const Variable &mask = makeVariable<bool>(Values{false}));
void sum_impl(const VariableProxy &summed, const VariableConstProxy &var);
void all_impl(const VariableProxy &out, const VariableConstProxy &var);
void any_impl(const VariableProxy &out, const VariableConstProxy &var);
void max_impl(const VariableProxy &out, const VariableConstProxy &var);
void min_impl(const VariableProxy &out, const VariableConstProxy &var);

} // namespace scipp::core

#endif // SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
