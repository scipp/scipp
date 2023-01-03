// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/variable/except.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::except {
VariableError::VariableError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const variable::Variable &expected,
                          const variable::Variable &actual,
                          const std::string &optional_message) {
  throw VariableError("Expected\n" + to_string(expected) + ", got\n" +
                      to_string(actual) + '.' + optional_message);
}
} // namespace scipp::except

namespace scipp::variable {

std::string pretty_dtype(const Variable &var) {
  if (!is_bins(var))
    return to_string(var.dtype());
  return to_string(var.dtype()) +
         "(dtype=" + pretty_dtype(variableFactory().elem_dtype(var)) + ")";
}

} // namespace scipp::variable
