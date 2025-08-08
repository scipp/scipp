// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/units/except.h"

namespace scipp::except {
UnitError::UnitError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const sc_units::Unit &expected,
                          const sc_units::Unit &actual,
                          const std::string &optional_message) {
  throw UnitError("Expected unit " + to_string(expected) + ", got " +
                  to_string(actual) + '.' + optional_message);
}
} // namespace scipp::except
