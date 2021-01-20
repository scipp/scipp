// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <optional>

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
empty_like(const VariableConstView &prototype,
           const std::optional<Dimensions> &shape = std::nullopt,
           const VariableConstView &sizes = {});

enum class SCIPP_VARIABLE_EXPORT FillValue { Zero, True, False, Max, Min };

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
full_like(const VariableConstView &prototype, const FillValue &fill);

} // namespace scipp::variable
