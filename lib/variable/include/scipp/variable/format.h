// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#pragma once

#include <string>

#include "scipp/core/format.h"

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {
struct SCIPP_VARIABLE_EXPORT VariableFormatSpec {
  bool show_type = true;
  core::FormatSpec nested{};
};

std::string SCIPP_VARIABLE_EXPORT
format_variable(const Variable &var, const VariableFormatSpec &spec,
                const core::FormatRegistry &formatters);
} // namespace scipp::variable
