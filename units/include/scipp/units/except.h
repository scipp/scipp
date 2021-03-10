// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-units_export.h"
#include "scipp/common/except.h"
#include "scipp/units/string.h"
#include "scipp/units/unit.h"

namespace scipp::except {

struct SCIPP_UNITS_EXPORT UnitError : public Error<units::Unit> {
  explicit UnitError(const std::string &msg);
};

SCIPP_UNITS_EXPORT UnitError mismatch_error(const units::Unit &expected,
                                            const units::Unit &actual);

//// We need deduction guides such that, e.g., the exception for a Variable
//// mismatch and VariableView mismatch are the same type.
// template <class T>
// MismatchError(const units::Unit &, const T &) -> MismatchError<units::Unit>;
// template <class T>
// MismatchError(const units::Dim &, const T &) -> MismatchError<units::Dim>;

} // namespace scipp::except
