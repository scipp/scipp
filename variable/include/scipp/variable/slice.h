// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Dim, scipp::index>
get_slice_params(const Dimensions &dims, const VariableConstView &coord,
                 const VariableConstView value);
[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Dim, scipp::index, scipp::index>
get_slice_params(const Dimensions &dims, const VariableConstView &coord,
                 const VariableConstView begin, const VariableConstView end);

} // namespace scipp::variable
