// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-units_export.h"
#include "scipp/common/except.h"
#include "scipp/units/string.h"
#include "scipp/units/unit.h"

namespace scipp::except {

struct SCIPP_UNITS_EXPORT UnitError : public Error<sc_units::Unit> {
  explicit UnitError(const std::string &msg);
};

template <>
[[noreturn]] SCIPP_UNITS_EXPORT void
throw_mismatch_error(const sc_units::Unit &expected,
                     const sc_units::Unit &actual,
                     const std::string &optional_message);

} // namespace scipp::except
