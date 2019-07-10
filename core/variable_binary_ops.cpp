// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

namespace scipp::core {

template <class T1, class T2> T1 &plus_equals(T1 &variable, const T2 &other) {
  // Addition with different Variable type is supported, mismatch of underlying
  // element types is handled in DataModel::operator+=.
  // Different name is ok for addition.
  expect::equals(variable.unit(), other.unit());
  expect::contains(variable.dims(), other.dims());
  // Note: This will broadcast/transpose the RHS if required. We do not support
  // changing the dimensions of the LHS though!
  transform_in_place<pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
      variable, other, [](auto &&a, auto &&b) { a += b; });
  return variable;
}

using arithmetic_type_pairs =
    std::tuple<std::pair<double, double>, std::pair<float, float>,
               std::pair<int64_t, int64_t>, std::pair<double, float>,
               std::pair<float, double>>;
using arithmetic_and_matrix_type_pairs = decltype(
    std::tuple_cat(std::declval<arithmetic_type_pairs>(),
                   std::tuple<std::pair<Eigen::Vector3d, Eigen::Vector3d>>()));

template <class T1, class T2> Variable plus(const T1 &a, const T2 &b) {
  expect::equals(a.unit(), b.unit());
  auto result = transform<arithmetic_and_matrix_type_pairs>(
      a, b, [](const auto a_, const auto b_) { return a_ + b_; });
  result.setUnit(a.unit());
  return result;
}

Variable Variable::operator-() const {
  auto result = transform<double, float, int64_t, Eigen::Vector3d>(
      *this, [](const auto a) { return -a; });
  result.setUnit(unit());
  return result;
}

Variable &Variable::operator+=(const Variable &other) & {
  return plus_equals(*this, other);
}
Variable &Variable::operator+=(const VariableConstProxy &other) & {
  return plus_equals(*this, other);
}
Variable &Variable::operator+=(const double value) & {
  // TODO By not setting a unit here this operator is only usable if the
  // variable is dimensionless. Should we ignore the unit for scalar operations,
  // i.e., set the same unit as *this.unit()?
  return plus_equals(*this, makeVariable<double>(value));
}

template <class T1, class T2> T1 &minus_equals(T1 &variable, const T2 &other) {
  expect::equals(variable.unit(), other.unit());
  expect::contains(variable.dims(), other.dims());
  transform_in_place<pair_self_t<double, float, int64_t, Eigen::Vector3d>>(
      variable, other, [](auto &&a, auto &&b) { a -= b; });
  return variable;
}

template <class T1, class T2> Variable minus(const T1 &a, const T2 &b) {
  expect::equals(a.unit(), b.unit());
  auto result = transform<arithmetic_and_matrix_type_pairs>(
      a, b, [](const auto a_, const auto b_) { return a_ - b_; });
  result.setUnit(a.unit());
  return result;
}

Variable &Variable::operator-=(const Variable &other) & {
  return minus_equals(*this, other);
}
Variable &Variable::operator-=(const VariableConstProxy &other) & {
  return minus_equals(*this, other);
}
Variable &Variable::operator-=(const double value) & {
  return minus_equals(*this, makeVariable<double>(value));
}

template <class T1, class T2> T1 &times_equals(T1 &variable, const T2 &other) {
  expect::contains(variable.dims(), other.dims());
  // setUnit is catching bad cases of changing units (if `variable` is a slice).
  const auto oldUnit = variable.unit();
  variable.setUnit(variable.unit() * other.unit());
  try {
    transform_in_place<pair_self_t<double, float, int64_t>,
                       pair_custom_t<std::pair<Eigen::Vector3d, double>>>(
        variable, other, [](auto &&a, auto &&b) { a *= b; });
  } catch (const std::runtime_error &) {
    variable.setUnit(oldUnit);
    throw;
  }
  return variable;
}

template <class T1, class T2> Variable times(const T1 &a, const T2 &b) {
  auto result = transform<arithmetic_type_pairs>(
      a, b, [](const auto a_, const auto b_) { return a_ * b_; });
  result.setUnit(a.unit() * b.unit());
  return result;
}

Variable &Variable::operator*=(const Variable &other) & {
  return times_equals(*this, other);
}
Variable &Variable::operator*=(const VariableConstProxy &other) & {
  return times_equals(*this, other);
}
Variable &Variable::operator*=(const double value) & {
  auto other = makeVariable<double>(value);
  other.setUnit(units::dimensionless);
  return times_equals(*this, other);
}

template <class T1, class T2> T1 &divide_equals(T1 &variable, const T2 &other) {
  expect::contains(variable.dims(), other.dims());
  // setUnit is catching bad cases of changing units (if `variable` is a slice).
  variable.setUnit(variable.unit() / other.unit());
  transform_in_place<pair_self_t<double, float, int64_t>,
                     pair_custom_t<std::pair<Eigen::Vector3d, double>>>(
      variable, other, [](auto &&a, auto &&b) { a /= b; });
  return variable;
}

template <class T1, class T2> Variable divide(const T1 &a, const T2 &b) {
  auto result = transform<arithmetic_type_pairs>(
      a, b, [](const auto a_, const auto b_) { return a_ / b_; });
  result.setUnit(a.unit() / b.unit());
  return result;
}

Variable &Variable::operator/=(const Variable &other) & {
  return divide_equals(*this, other);
}
Variable &Variable::operator/=(const VariableConstProxy &other) & {
  return divide_equals(*this, other);
}
Variable &Variable::operator/=(const double value) & {
  return divide_equals(*this, makeVariable<double>(value));
}

VariableProxy VariableProxy::operator+=(const Variable &other) const {
  return plus_equals(*this, other);
}
VariableProxy VariableProxy::operator+=(const VariableConstProxy &other) const {
  return plus_equals(*this, other);
}
VariableProxy VariableProxy::operator+=(const double value) const {
  return plus_equals(*this, makeVariable<double>(value));
}

VariableProxy VariableProxy::operator-=(const Variable &other) const {
  return minus_equals(*this, other);
}
VariableProxy VariableProxy::operator-=(const VariableConstProxy &other) const {
  return minus_equals(*this, other);
}
VariableProxy VariableProxy::operator-=(const double value) const {
  return minus_equals(*this, makeVariable<double>(value));
}

VariableProxy VariableProxy::operator*=(const Variable &other) const {
  return times_equals(*this, other);
}
VariableProxy VariableProxy::operator*=(const VariableConstProxy &other) const {
  return times_equals(*this, other);
}
VariableProxy VariableProxy::operator*=(const double value) const {
  return times_equals(*this, makeVariable<double>(value));
}

VariableProxy VariableProxy::operator/=(const Variable &other) const {
  return divide_equals(*this, other);
}
VariableProxy VariableProxy::operator/=(const VariableConstProxy &other) const {
  return divide_equals(*this, other);
}
VariableProxy VariableProxy::operator/=(const double value) const {
  return divide_equals(*this, makeVariable<double>(value));
}

Variable VariableConstProxy::operator-() const {
  Variable copy(*this);
  return -copy;
}

Variable operator+(const Variable &a, const Variable &b) { return plus(a, b); }
Variable operator-(const Variable &a, const Variable &b) { return minus(a, b); }
Variable operator*(const Variable &a, const Variable &b) { return times(a, b); }
Variable operator/(const Variable &a, const Variable &b) {
  return divide(a, b);
}
Variable operator+(const Variable &a, const VariableConstProxy &b) {
  return plus(a, b);
}
Variable operator-(const Variable &a, const VariableConstProxy &b) {
  return minus(a, b);
}
Variable operator*(const Variable &a, const VariableConstProxy &b) {
  return times(a, b);
}
Variable operator/(const Variable &a, const VariableConstProxy &b) {
  return divide(a, b);
}
Variable operator+(const VariableConstProxy &a, const Variable &b) {
  return plus(a, b);
}
Variable operator-(const VariableConstProxy &a, const Variable &b) {
  return minus(a, b);
}
Variable operator*(const VariableConstProxy &a, const Variable &b) {
  return times(a, b);
}
Variable operator/(const VariableConstProxy &a, const Variable &b) {
  return divide(a, b);
}
Variable operator+(const VariableConstProxy &a, const VariableConstProxy &b) {
  return plus(a, b);
}
Variable operator-(const VariableConstProxy &a, const VariableConstProxy &b) {
  return minus(a, b);
}
Variable operator*(const VariableConstProxy &a, const VariableConstProxy &b) {
  return times(a, b);
}
Variable operator/(const VariableConstProxy &a, const VariableConstProxy &b) {
  return divide(a, b);
}
// Note: The std::move here is necessary because RVO does not work for variables
// that are function parameters.
Variable operator+(Variable a, const double b) { return std::move(a += b); }
Variable operator-(Variable a, const double b) { return std::move(a -= b); }
Variable operator*(Variable a, const double b) { return std::move(a *= b); }
Variable operator/(Variable a, const double b) { return std::move(a /= b); }
Variable operator+(const double a, Variable b) { return std::move(b += a); }
Variable operator-(const double a, Variable b) { return -(b -= a); }
Variable operator*(const double a, Variable b) { return std::move(b *= a); }
Variable operator/(const double a, Variable b) {
  b.setUnit(units::Unit(units::dimensionless) / b.unit());
  transform_in_place<double, float>(
      b, overloaded{[a](double &b_) { b_ = a / b_; },
                    [a](float &b_) { b_ = static_cast<float>(a / b_); },
                    [a](auto &b_) {
                      b_ = static_cast<std::remove_reference_t<decltype(b_)>>(
                          a / b_);
                    }});
  return b;
}

} // namespace scipp::core
