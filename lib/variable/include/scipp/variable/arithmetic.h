// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-variable_export.h"
#include "scipp/variable/generated_arithmetic.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable operator+(const Variable &a,
                                                       const Variable &b);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable operator-(const Variable &a,
                                                       const Variable &b);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable operator*(const Variable &a,
                                                       const Variable &b);
[[nodiscard]] SCIPP_VARIABLE_EXPORT Variable operator/(const Variable &a,
                                                       const Variable &b);

SCIPP_VARIABLE_EXPORT Variable &operator+=(Variable &a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable &operator-=(Variable &a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable &operator*=(Variable &a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable &operator/=(Variable &a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable &floor_divide_equals(Variable &a,
                                                    const Variable &b);

SCIPP_VARIABLE_EXPORT Variable operator+=(Variable &&a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable operator-=(Variable &&a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable operator*=(Variable &&a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable operator/=(Variable &&a, const Variable &b);
SCIPP_VARIABLE_EXPORT Variable floor_divide_equals(Variable &&a,
                                                   const Variable &b);

} // namespace scipp::variable

namespace scipp::sc_units {
template <typename T>
std::enable_if_t<std::is_arithmetic_v<T> ||
                     std::is_same_v<T, scipp::core::time_point>,
                 Variable>
operator*(T v, const sc_units::Unit &unit) {
  return makeVariable<T>(sc_units::Unit{unit}, Values{v});
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T> ||
                     std::is_same_v<T, scipp::core::time_point>,
                 Variable>
operator/(T v, const sc_units::Unit &unit) {
  return makeVariable<T>(sc_units::one / unit, Values{v});
}
} // namespace scipp::sc_units
