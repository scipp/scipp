// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include "scipp/units/except.h"

namespace scipp::except {
UnitError::UnitError(const std::string &msg) : Error{msg} {}

UnitError mismatch_error(const units::Unit &expected,
                         const units::Unit &actual) {
  return UnitError("Expected unit " + to_string(expected) + ", got " +
                   to_string(actual) + '.');
}
} // namespace scipp::except