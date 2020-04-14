// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_VARIABLE_SUBSPAN_VIEW_H
#define SCIPP_VARIABLE_SUBSPAN_VIEW_H

#include <scipp/variable/variable.h>

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable subspan_view(Variable &var, const Dim dim);
SCIPP_VARIABLE_EXPORT Variable subspan_view(const VariableView &var,
                                            const Dim dim);
SCIPP_VARIABLE_EXPORT Variable subspan_view(const VariableConstView &var,
                                            const Dim dim);

} // namespace scipp::variable

#endif // SCIPP_VARIABLE_SUBSPAN_VIEW_H
