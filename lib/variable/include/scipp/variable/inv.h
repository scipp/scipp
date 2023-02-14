// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable inv(const Variable &var);
}
