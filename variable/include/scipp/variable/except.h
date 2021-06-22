// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <stdexcept>

#include "scipp-variable_export.h"
#include "scipp/common/except.h"
#include "scipp/core/except.h"
#include "scipp/variable/string.h"

namespace scipp::except {

struct SCIPP_VARIABLE_EXPORT VariableError : public Error<variable::Variable> {
  explicit VariableError(const std::string &msg);
};

template <>
[[noreturn]] SCIPP_VARIABLE_EXPORT void
throw_mismatch_error(const variable::Variable &expected,
                     const variable::Variable &actual,
                     const std::string &optional_message);

} // namespace scipp::except

namespace scipp::variable {
SCIPP_VARIABLE_EXPORT std::string pretty_dtype(const Variable &var);
}
