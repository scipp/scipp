// SPDX-License-Identifier: BSD-3-Clause
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
isclose(const Variable &a, const Variable &b, const Variable &rtol,
        const Variable &atol,
        const NanComparisons equal_nans = NanComparisons::NotEqual);

} // namespace scipp::variable
