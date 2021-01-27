// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

enum class CumSumMode { Exclusive, Inclusive };

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum(const VariableConstView &var, const Dim dim,
       const CumSumMode mode = CumSumMode::Inclusive);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum(const VariableConstView &var,
       const CumSumMode mode = CumSumMode::Inclusive);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum_bins(const VariableConstView &var,
            const CumSumMode mode = CumSumMode::Inclusive);

} // namespace scipp::variable

namespace scipp {
using variable::CumSumMode;
}
