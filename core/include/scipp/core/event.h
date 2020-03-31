// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_EVENT_H
#define SCIPP_CORE_EVENT_H

#include "scipp/core/dataset.h"

namespace scipp::core::event {

SCIPP_CORE_EXPORT void append(const VariableView &a,
                              const VariableConstView &b);
SCIPP_CORE_EXPORT void append(const DataArrayView &a,
                              const DataArrayConstView &b);
SCIPP_CORE_EXPORT Variable concatenate(const VariableConstView &a,
                                       const VariableConstView &b);
SCIPP_CORE_EXPORT DataArray concatenate(const DataArrayConstView &a,
                                        const DataArrayConstView &b);
SCIPP_CORE_EXPORT Variable broadcast(const VariableConstView &dense,
                                     const VariableConstView &shape);
SCIPP_CORE_EXPORT Variable broadcast_weights(const DataArrayConstView &events);
SCIPP_CORE_EXPORT Variable sizes(const VariableConstView &events);
SCIPP_CORE_EXPORT void reserve(const VariableView &events,
                               const VariableConstView &capacity);

[[nodiscard]] SCIPP_CORE_EXPORT DataArray
filter(const DataArrayConstView &array, const Dim dim,
       const VariableConstView &interval,
       const AttrPolicy attrPolicy = AttrPolicy::Keep);

} // namespace scipp::core::event

#endif // SCIPP_CORE_EVENT_H
