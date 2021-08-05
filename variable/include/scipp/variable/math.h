// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

#include "scipp/variable/generated_math.h"

namespace scipp::variable {
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable pow(const Variable &base,
                                                 const Variable &exponent);
}