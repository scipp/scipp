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

SCIPP_CORE_EXPORT Variable astype(const VariableConstView &var,
                                  const DType type);

[[nodiscard]] SCIPP_CORE_EXPORT Variable
reciprocal(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable reciprocal(Variable &&var);
SCIPP_CORE_EXPORT VariableView reciprocal(const VariableConstView &var,
                                          const VariableView &out);

SCIPP_CORE_EXPORT std::vector<Variable>
split(const Variable &var, const Dim dim,
      const std::vector<scipp::index> &indices);
[[nodiscard]] SCIPP_CORE_EXPORT Variable abs(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable abs(Variable &&var);
SCIPP_CORE_EXPORT VariableView abs(const VariableConstView &var,
                                   const VariableView &out);
SCIPP_CORE_EXPORT Variable broadcast(const VariableConstView &var,
                                     const Dimensions &dims);
SCIPP_CORE_EXPORT Variable concatenate(const VariableConstView &a1,
                                       const VariableConstView &a2,
                                       const Dim dim);
SCIPP_CORE_EXPORT Variable dot(const Variable &a, const Variable &b);
SCIPP_CORE_EXPORT Variable filter(const Variable &var, const Variable &filter);
[[nodiscard]] SCIPP_CORE_EXPORT Variable mean(const VariableConstView &var,
                                              const Dim dim);
SCIPP_CORE_EXPORT VariableView mean(const VariableConstView &var, const Dim dim,
                                    const VariableView &out);
SCIPP_CORE_EXPORT Variable norm(const VariableConstView &var);
SCIPP_CORE_EXPORT Variable permute(const Variable &var, const Dim dim,
                                   const std::vector<scipp::index> &indices);
SCIPP_CORE_EXPORT Variable rebin(const VariableConstView &var, const Dim dim,
                                 const VariableConstView &oldCoord,
                                 const VariableConstView &newCoord);
SCIPP_CORE_EXPORT Variable resize(const VariableConstView &var, const Dim dim,
                                  const scipp::index size);
SCIPP_CORE_EXPORT Variable reverse(Variable var, const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable sqrt(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable sqrt(Variable &&var);
SCIPP_CORE_EXPORT VariableView sqrt(const VariableConstView &var,
                                    const VariableView &out);

[[nodiscard]] SCIPP_CORE_EXPORT Variable flatten(const VariableConstView &var,
                                                 const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable sum(const VariableConstView &var,
                                             const Dim dim);
SCIPP_CORE_EXPORT VariableView sum(const VariableConstView &var, const Dim dim,
                                   const VariableView &out);

SCIPP_CORE_EXPORT Variable copy(const VariableConstView &var);

// Logical reductions
[[nodiscard]] SCIPP_CORE_EXPORT Variable any(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable any(const VariableConstView &var,
                                             const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable all(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable all(const VariableConstView &var,
                                             const Dim dim);

// Other reductions
[[nodiscard]] SCIPP_CORE_EXPORT Variable max(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable max(const VariableConstView &var,
                                             const Dim dim);
[[nodiscard]] SCIPP_CORE_EXPORT Variable min(const VariableConstView &var);
[[nodiscard]] SCIPP_CORE_EXPORT Variable min(const VariableConstView &var,
                                             const Dim dim);

SCIPP_CORE_EXPORT VariableView nan_to_num(const VariableConstView &var,
                                          const VariableConstView &replacement,
                                          const VariableView &out);
SCIPP_CORE_EXPORT VariableView positive_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement,
    const VariableView &out);
SCIPP_CORE_EXPORT VariableView negative_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement,
    const VariableView &out);

[[nodiscard]] SCIPP_CORE_EXPORT Variable
nan_to_num(const VariableConstView &var, const VariableConstView &replacement);
[[nodiscard]] SCIPP_CORE_EXPORT Variable pos_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement);
[[nodiscard]] SCIPP_CORE_EXPORT Variable neg_inf_to_num(
    const VariableConstView &var, const VariableConstView &replacement);
namespace geometry {
[[nodiscard]] SCIPP_CORE_EXPORT Variable position(const VariableConstView &x,
                                                  const VariableConstView &y,
                                                  const VariableConstView &z);
[[nodiscard]] SCIPP_CORE_EXPORT Variable x(const VariableConstView &pos);
[[nodiscard]] SCIPP_CORE_EXPORT Variable y(const VariableConstView &pos);
[[nodiscard]] SCIPP_CORE_EXPORT Variable z(const VariableConstView &pos);

} // namespace geometry

} // namespace scipp::core
