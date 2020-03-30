// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/dataset.h"

namespace scipp::core::unaligned {

SCIPP_CORE_EXPORT DataArray
realign(DataArray unaligned, std::vector<std::pair<Dim, Variable>> coords);
SCIPP_CORE_EXPORT Dataset realign(Dataset unaligned,
                                  std::vector<std::pair<Dim, Variable>> coords);

SCIPP_CORE_EXPORT bool is_realigned_events(const DataArrayConstView &realigned);
SCIPP_CORE_EXPORT Dim realigned_event_dim(const DataArrayConstView &realigned);
SCIPP_CORE_EXPORT VariableConstView
realigned_event_coord(const DataArrayConstView &realigned);

SCIPP_CORE_EXPORT
DataArray
filter_recurse(DataArray &&unaligned,
               const scipp::span<const std::pair<Dim, Variable>> bounds);
SCIPP_CORE_EXPORT
DataArray
filter_recurse(const DataArrayConstView &unaligned,
               const scipp::span<const std::pair<Dim, Variable>> bounds,
               const AttrPolicy attrPolicy = AttrPolicy::Keep);

} // namespace scipp::core::unaligned
