// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <stdexcept>

#include "scipp-variable_export.h"
#include "scipp/common/except.h"
#include "scipp/core/except.h"
#include "scipp/variable/string.h"

namespace scipp::except {

using VariableError = Error<variable::Variable>;
using VariableMismatchError = MismatchError<variable::Variable>;

template <class T>
MismatchError(const variable::VariableConstView &, const T &)
    -> MismatchError<variable::Variable>;

} // namespace scipp::except
