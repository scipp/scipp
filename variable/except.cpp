// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/variable/except.h"

namespace scipp::except {
VariableError::VariableError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const variable::Variable &expected,
                          const variable::Variable &actual) {
  throw VariableError("Expected Variable " + to_string(expected) + ", got " +
                      to_string(actual) + '.');
}
} // namespace scipp::except
