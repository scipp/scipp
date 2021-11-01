// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include "scipp/core/flags.h"

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

SCIPP_VARIABLE_EXPORT Variable astype(const Variable &var, DType type,
                                      CopyPolicy copy = CopyPolicy::Always);

} // namespace scipp::variable
