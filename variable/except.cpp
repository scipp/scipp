// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/variable/except.h"

namespace scipp::except {
VariableError::VariableError(const std::string &msg) : Error{msg} {}

VariableError mismatch_error(const variable::Variable &expected,
                             const variable::Variable &actual) {
  return VariableError("Expected Variable " + to_string(expected) + ", got " +
                       to_string(actual) + '.');
}
} // namespace scipp::except