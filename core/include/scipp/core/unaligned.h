// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_UNALIGNED_H
#define SCIPP_CORE_UNALIGNED_H

#include "scipp/core/dataset.h"

namespace scipp::core::unaligned {

Dim unaligned_dim(const VariableConstView &unaligned);

DataArray realign(DataArray unaligned,
                  std::vector<std::pair<Dim, Variable>> coords);

} // namespace scipp::core::unaligned

#endif // SCIPP_CORE_EVENT_H
