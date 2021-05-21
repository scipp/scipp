// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

enum class CumSumMode { Exclusive, Inclusive };

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum(const Variable &var, const Dim dim,
       const CumSumMode mode = CumSumMode::Inclusive);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum(const Variable &var, const CumSumMode mode = CumSumMode::Inclusive);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
cumsum_bins(const Variable &var, const CumSumMode mode = CumSumMode::Inclusive);

} // namespace scipp::variable

namespace scipp {
using variable::CumSumMode;
}
