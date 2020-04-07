// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/variable.h"

namespace scipp::core {

SCIPP_CORE_EXPORT Variable operator+(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator-(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator*(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator/(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator|(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator&(const VariableConstView &a,
                                     const VariableConstView &b);
SCIPP_CORE_EXPORT Variable operator^(const VariableConstView &a,
                                     const VariableConstView &b);
// Note: If the left-hand-side in an addition is a VariableView this simply
// implicitly converts it to a Variable. A copy for the return value is required
// anyway so this is a convenient way to avoid defining more overloads.
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator+(const T value, const VariableConstView &a) {
  return makeVariable<T>(Values{value}) + a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator-(const T value, const VariableConstView &a) {
  return makeVariable<T>(Values{value}) - a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator*(const T value, const VariableConstView &a) {
  return makeVariable<T>(Values{value}) * a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator/(const T value, const VariableConstView &a) {
  return makeVariable<T>(Values{value}) / a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator+(const VariableConstView &a, const T value) {
  return a + makeVariable<T>(Values{value});
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator-(const VariableConstView &a, const T value) {
  return a - makeVariable<T>(Values{value});
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator*(const VariableConstView &a, const T value) {
  return a * makeVariable<T>(Values{value});
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Variable operator/(const VariableConstView &a, const T value) {
  return a / makeVariable<T>(Values{value});
}

template <class T>
Variable operator*(Variable a, const boost::units::quantity<T> &quantity) {
  return std::move(a *= quantity);
}
template <class T>
Variable operator/(Variable a, const boost::units::quantity<T> &quantity) {
  return std::move(a /= quantity);
}
template <class T>
Variable operator/(const boost::units::quantity<T> &quantity, Variable a) {
  return makeVariable<double>(Dimensions{}, units::Unit(T{}),
                              Values{quantity.value()}) /
         std::move(a);
}

} // namespace scipp::core

namespace scipp::units {
template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Variable>
operator*(T v, const units::Unit &unit) {
  return makeVariable<T>(Dimensions{}, units::Unit{unit}, Values{v});
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Variable>
operator/(T v, const units::Unit &unit) {
  return makeVariable<T>(Dimensions{}, units::Unit(units::dimensionless) / unit,
                         Values{v});
}
} // namespace scipp::units
