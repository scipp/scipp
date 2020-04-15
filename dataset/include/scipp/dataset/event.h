// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset::event {

SCIPP_DATASET_EXPORT void append(const DataArrayView &a,
                                 const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray concatenate(const DataArrayConstView &a,
                                           const DataArrayConstView &b);
SCIPP_DATASET_EXPORT Variable
broadcast_weights(const DataArrayConstView &events);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
filter(const DataArrayConstView &array, const Dim dim,
       const VariableConstView &interval,
       const AttrPolicy attrPolicy = AttrPolicy::Keep);

[[nodiscard]] SCIPP_CORE_EXPORT Variable map(const DataArrayConstView &function,
                                             const VariableConstView &x,
                                             Dim dim = Dim::Invalid);

} // namespace scipp::dataset::event
