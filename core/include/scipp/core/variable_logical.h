// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/variable.h"

namespace scipp::core {

SCIPP_CORE_EXPORT Variable operator|(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator&(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator^(const VariableConstView &a,
                                     const VariableConstView &b);

} // namespace scipp::core
