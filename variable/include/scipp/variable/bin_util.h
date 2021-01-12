// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

//[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable make_bins(Variable indices,
//                                                       const Dim dim,
//                                                       Variable buffer);
//

[[nodiscard]] SCIPP_VARIABLE_EXPORT std::tuple<Variable, Variable>
subbin_offsets(const VariableConstView &start_, const VariableConstView &stop_,
               const VariableConstView &subbin_sizes_, const scipp::index nsrc,
               const scipp::index ndst, const scipp::index nbin);

} // namespace scipp::variable
