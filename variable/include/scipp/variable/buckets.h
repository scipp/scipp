// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT void copy_slices(const VariableConstView &src,
                                       const VariableView &dst, const Dim dim,
                                       const VariableConstView &srcIndices,
                                       const VariableConstView &dstIndices);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable resize_default_init(
    const VariableConstView &var, const Dim dim, const scipp::index size);

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Variable, scipp::index>
sizes_to_begin(const VariableConstView &sizes);

} // namespace scipp::variable
