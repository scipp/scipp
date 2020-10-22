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

} // namespace scipp::variable
