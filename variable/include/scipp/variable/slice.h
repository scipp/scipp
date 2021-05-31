// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Dim, scipp::index>
get_slice_params(const Sizes &sizes, const Variable &coord,
                 const Variable &value);
[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Dim, scipp::index, scipp::index>
get_slice_params(const Sizes &sizes, const Variable &coord,
                 const Variable &begin, const Variable &end);

} // namespace scipp::variable
