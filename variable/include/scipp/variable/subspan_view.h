// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_SUBSPAN_VIEW_H
#define SCIPP_CORE_SUBSPAN_VIEW_H

#include <scipp/core/variable.h>

namespace scipp::core {

SCIPP_CORE_EXPORT Variable subspan_view(Variable &var, const Dim dim);
SCIPP_CORE_EXPORT Variable subspan_view(const VariableView &var, const Dim dim);
SCIPP_CORE_EXPORT Variable subspan_view(const VariableConstView &var,
                                        const Dim dim);

} // namespace scipp::core

#endif // SCIPP_CORE_SUBSPAN_VIEW_H
