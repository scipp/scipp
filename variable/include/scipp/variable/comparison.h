// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

/// Functions to provide numpy-like element-wise comparison
/// between two Variables.

SCIPP_VARIABLE_EXPORT Variable less(const VariableConstView &x,
                                    const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable greater(const VariableConstView &x,
                                       const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable greater_equal(const VariableConstView &x,
                                             const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable less_equal(const VariableConstView &x,
                                          const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable equal(const VariableConstView &x,
                                     const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable not_equal(const VariableConstView &x,
                                         const VariableConstView &y);

SCIPP_VARIABLE_EXPORT Variable is_approx(const VariableConstView &a,
                                         const VariableConstView &b,
                                         const VariableConstView &t);

} // namespace scipp::variable
