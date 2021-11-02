// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable bin_sizes(const Variable &var);

SCIPP_VARIABLE_EXPORT void copy_slices(const Variable &src, Variable dst,
                                       const Dim dim,
                                       const Variable &srcIndices,
                                       const Variable &dstIndices);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable resize_default_init(
    const Variable &var, const Dim dim, const scipp::index size);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable make_bins(Variable indices,
                                                       const Dim dim,
                                                       Variable buffer);

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
make_bins_no_validate(Variable indices, const Dim dim, Variable buffer);

} // namespace scipp::variable
