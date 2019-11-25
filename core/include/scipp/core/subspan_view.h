// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_SUBSPAN_VIEW_H
#define SCIPP_CORE_SUBSPAN_VIEW_H

#include <scipp/core/variable.h>

namespace scipp::core {

SCIPP_CORE_EXPORT Variable subspan_view(Variable &var, const Dim dim);
SCIPP_CORE_EXPORT Variable subspan_view(const VariableProxy &var,
                                        const Dim dim);
SCIPP_CORE_EXPORT Variable subspan_view(const VariableConstProxy &var,
                                        const Dim dim);

} // namespace scipp::core

#endif // SCIPP_CORE_SUBSPAN_VIEW_H
