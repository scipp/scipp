// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include "scipp/variable/generated_comparison.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

/// Functions to provide numpy-like element-wise comparison
/// between two Variables.
enum class NanComparisons { Equal, NotEqual };

SCIPP_VARIABLE_EXPORT Variable
is_close(const VariableConstView &a, const VariableConstView &b,
         const VariableConstView rtol, const VariableConstView atol,
         const NanComparisons equal_nans = NanComparisons::NotEqual);

} // namespace scipp::variable
