// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Samuel Jones
#pragma once

#include "scipp/common/index.h"
#include "scipp/variable/variable.h"

using namespace scipp::variable;

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable floor(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable ceil(const Variable &var);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable
round(const Variable &var, const scipp::index decimals = 0);