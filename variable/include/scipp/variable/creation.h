// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <optional>

#include "scipp/core/flags.h"

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
empty(const Dimensions &dims, const units::Unit &unit, const DType type,
      const bool with_variances = false);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
ones(const Dimensions &dims, const units::Unit &unit, const DType type,
     const bool with_variances = false);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
empty_like(const Variable &prototype,
           const std::optional<Dimensions> &shape = std::nullopt,
           const Variable &sizes = {});

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
special_like(const Variable &prototype, const FillValue &fill);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
zero_like(const Variable &prototype);

} // namespace scipp::variable
