// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <scipp/variable/variable.h>

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable subspan_view(Variable &var,
                                                          const Dim dim);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable subspan_view(const Variable &var,
                                                          const Dim dim);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
subspan_view(Variable &var, const Dim dim, const Variable &indices);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
subspan_view(const Variable &var, const Dim dim, const Variable &indices);

} // namespace scipp::variable
