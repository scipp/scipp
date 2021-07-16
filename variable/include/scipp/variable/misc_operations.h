// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable::geometry {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable position(const Variable &x,
                                                      const Variable &y,
                                                      const Variable &z);

} // namespace scipp::variable::geometry
