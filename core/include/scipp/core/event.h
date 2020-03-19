// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_EVENT_H
#define SCIPP_CORE_EVENT_H

#include "scipp/core/variable.h"

namespace scipp::core {

class DataArrayConstView;

bool is_events(const VariableConstView &var);
bool is_events(const DataArrayConstView &array);

namespace event {
SCIPP_CORE_EXPORT Variable concatenate(const VariableConstView &a,
                                       const VariableConstView &b);
}

} // namespace scipp::core

#endif // SCIPP_CORE_EVENT_H
