// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT VariableConstView
left_edge(const VariableConstView &edges);
[[nodiscard]] SCIPP_VARIABLE_EXPORT VariableConstView
right_edge(const VariableConstView &edges);

} // namespace scipp::variable
