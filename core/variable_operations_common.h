// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
#define SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H

namespace scipp::core {

void flatten_impl(const VariableProxy &summed, const VariableConstProxy &var);
void sum_impl(const VariableProxy &summed, const VariableConstProxy &var);

} // namespace scipp::core

#endif // SCIPP_CORE_VARIABLE_OPERATIONS_COMMON_H
