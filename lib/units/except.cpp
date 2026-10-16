// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/units/except.h"

#include <format>

namespace scipp::except {
UnitError::UnitError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const sc_units::Unit &expected,
                          const sc_units::Unit &actual,
                          const std::string &optional_message) {
  throw UnitError("Expected unit " + to_string(expected) + ", got " +
                  to_string(actual) + '.' + optional_message);
}

void expect_same_dimension(const sc_units::Unit &a, const sc_units::Unit &b,
                           const char *const op) {
  if (!a.has_same_base(b) && (a.has_value() || b.has_value())) {
    throw UnitError(std::format("Expected units to have the same physical "
                                "dimension in operation '{}', got {} and {}.",
                                op, a.name(), b.name()));
  }
}
} // namespace scipp::except
