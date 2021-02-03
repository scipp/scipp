// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/generated_arithmetic.h"
#include "scipp/variable/variable.h"

namespace scipp::units {
template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Variable>
operator*(T v, const units::Unit &unit) {
  return makeVariable<T>(units::Unit{unit}, Values{v});
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Variable>
operator/(T v, const units::Unit &unit) {
  return makeVariable<T>(units::one / unit, Values{v});
}
} // namespace scipp::units
