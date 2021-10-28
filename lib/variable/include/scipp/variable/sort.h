// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#pragma once

#include "scipp/core/flags.h"

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable sort(const Variable &var,
                                                  const Dim dim,
                                                  const SortOrder order);

} // namespace scipp::variable
