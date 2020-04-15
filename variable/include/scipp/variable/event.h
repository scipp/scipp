// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_VARIABLE_EVENT_H
#define SCIPP_VARIABLE_EVENT_H

#include "scipp/variable/variable.h"

namespace scipp::variable::event {

SCIPP_VARIABLE_EXPORT void append(const VariableView &a,
                                  const VariableConstView &b);
SCIPP_VARIABLE_EXPORT Variable concatenate(const VariableConstView &a,
                                           const VariableConstView &b);
SCIPP_VARIABLE_EXPORT Variable broadcast(const VariableConstView &dense,
                                         const VariableConstView &shape);
SCIPP_VARIABLE_EXPORT Variable sizes(const VariableConstView &events);
SCIPP_VARIABLE_EXPORT void reserve(const VariableView &events,
                                   const VariableConstView &capacity);

} // namespace scipp::variable::event

#endif // SCIPP_VARIABLE_EVENT_H
