// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include "operators.h"

namespace scipp::core {

static constexpr auto or_op_ = overloaded{
    [](const auto &var_, const auto &other_) -> bool { return var_ | other_; },
    dimensionless_unit_check_return};

static constexpr auto or_equals_ =
    overloaded{[](auto &var_, const auto &other_) { var_ |= other_; },
               dimensionless_unit_check};

static constexpr auto and_op_ = overloaded{
    [](const auto &var_, const auto &other_) -> bool { return var_ & other_; },
    dimensionless_unit_check_return};

static constexpr auto and_equals_ =
    overloaded{[](auto &var_, const auto &other_) { var_ &= other_; },
               dimensionless_unit_check};

static constexpr auto xor_op_ = overloaded{
    [](const auto &var_, const auto &other_) -> bool { return var_ ^ other_; },
    dimensionless_unit_check_return};

static constexpr auto xor_equals_ =
    overloaded{[](auto &var_, const auto &other_) { var_ ^= other_; },
               dimensionless_unit_check};

template <class T1, class T2> T1 &or_equals(T1 &variable, const T2 &other) {
  transform_in_place<pair_self_t<bool>>(variable, other, or_equals_);
  return variable;
}

template <class T1, class T2> Variable or_op(const T1 &a, const T2 &b) {
  return transform<pair_self_t<bool>>(a, b, or_op_);
}

template <class T1, class T2> T1 &and_equals(T1 &variable, const T2 &other) {
  transform_in_place<pair_self_t<bool>>(variable, other, and_equals_);
  return variable;
}

template <class T1, class T2> Variable and_op(const T1 &a, const T2 &b) {
  return transform<pair_self_t<bool>>(a, b, and_op_);
}

template <class T1, class T2> T1 &xor_equals(T1 &variable, const T2 &other) {
  transform_in_place<pair_self_t<bool>>(variable, other, xor_equals_);
  return variable;
}

template <class T1, class T2> Variable xor_op(const T1 &a, const T2 &b) {
  return transform<pair_self_t<bool>>(a, b, xor_op_);
}

Variable &Variable::operator|=(const Variable &other) & {
  return or_equals(*this, other);
}
Variable &Variable::operator|=(const VariableConstProxy &other) & {
  return or_equals(*this, other);
}

Variable &Variable::operator&=(const Variable &other) & {
  return and_equals(*this, other);
}
Variable &Variable::operator&=(const VariableConstProxy &other) & {
  return and_equals(*this, other);
}

Variable &Variable::operator^=(const Variable &other) & {
  return xor_equals(*this, other);
}
Variable &Variable::operator^=(const VariableConstProxy &other) & {
  return xor_equals(*this, other);
}

VariableProxy VariableProxy::operator|=(const Variable &other) const {
  return or_equals(*this, other);
}
VariableProxy VariableProxy::operator|=(const VariableConstProxy &other) const {
  return or_equals(*this, other);
}

VariableProxy VariableProxy::operator&=(const Variable &other) const {
  return and_equals(*this, other);
}
VariableProxy VariableProxy::operator&=(const VariableConstProxy &other) const {
  return and_equals(*this, other);
}

VariableProxy VariableProxy::operator^=(const Variable &other) const {
  return xor_equals(*this, other);
}
VariableProxy VariableProxy::operator^=(const VariableConstProxy &other) const {
  return xor_equals(*this, other);
}

Variable operator|(const Variable &a, const Variable &b) { return or_op(a, b); }
Variable operator&(const Variable &a, const Variable &b) {
  return and_op(a, b);
}
Variable operator^(const Variable &a, const Variable &b) {
  return xor_op(a, b);
}
Variable operator|(const Variable &a, const VariableConstProxy &b) {
  return or_op(a, b);
}
Variable operator&(const Variable &a, const VariableConstProxy &b) {
  return and_op(a, b);
}
Variable operator^(const Variable &a, const VariableConstProxy &b) {
  return xor_op(a, b);
}
Variable operator|(const VariableConstProxy &a, const Variable &b) {
  return or_op(a, b);
}
Variable operator&(const VariableConstProxy &a, const Variable &b) {
  return and_op(a, b);
}
Variable operator^(const VariableConstProxy &a, const Variable &b) {
  return xor_op(a, b);
}
Variable operator|(const VariableConstProxy &a, const VariableConstProxy &b) {
  return or_op(a, b);
}
Variable operator&(const VariableConstProxy &a, const VariableConstProxy &b) {
  return and_op(a, b);
}
Variable operator^(const VariableConstProxy &a, const VariableConstProxy &b) {
  return xor_op(a, b);
}

Variable Variable::operator~() const {
  return transform<bool>(
      *this, overloaded{[](const auto &current) { return !current; },
                        [](const units::Unit &unit) -> units::Unit {
                          expect::equals(unit, units::dimensionless);
                          return unit;
                        }});
}

} // namespace scipp::core
