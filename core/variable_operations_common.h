// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
#define SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H

#include "scipp/core/variable.h"

namespace scipp::core {

// Helpers for in-place reductions and reductions with groupby.
void flatten_impl(const VariableView &summed, const VariableConstView &var,
                  const VariableConstView &mask);
void sum_impl(const VariableView &summed, const VariableConstView &var);
void all_impl(const VariableView &out, const VariableConstView &var);
void any_impl(const VariableView &out, const VariableConstView &var);
void max_impl(const VariableView &out, const VariableConstView &var);
void min_impl(const VariableView &out, const VariableConstView &var);

} // namespace scipp::core

#endif // SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
