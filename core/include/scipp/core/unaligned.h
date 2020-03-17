// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/dataset.h"

namespace scipp::core::unaligned {

Dim unaligned_dim(const VariableConstView &unaligned);

DataArray realign(DataArray unaligned,
                  std::vector<std::pair<Dim, Variable>> coords);

} // namespace scipp::core::unaligned
