// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arithmetic.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

template <class T1, class T2> T1 &plus_equals(T1 &variable, const T2 &other) {
  // Note: This will broadcast/transpose the RHS if required. We do not support
  // changing the dimensions of the LHS though!
  transform_in_place(variable, other, core::element::plus_equals);
  return variable;
}

Variable &Variable::operator+=(const VariableConstView &other) & {
  VariableView(*this) += other;
  return *this;
}

template <class T1, class T2> T1 &minus_equals(T1 &variable, const T2 &other) {
  transform_in_place(variable, other, core::element::minus_equals);
  return variable;
}

Variable &Variable::operator-=(const VariableConstView &other) & {
  VariableView(*this) -= other;
  return *this;
}

template <class T1, class T2> T1 &times_equals(T1 &variable, const T2 &other) {
  transform_in_place(variable, other, core::element::times_equals);
  return variable;
}

Variable &Variable::operator*=(const VariableConstView &other) & {
  VariableView(*this) *= other;
  return *this;
}

template <class T1, class T2> T1 &divide_equals(T1 &variable, const T2 &other) {
  transform_in_place(variable, other, core::element::divide_equals);
  return variable;
}

Variable &Variable::operator/=(const VariableConstView &other) & {
  VariableView(*this) /= other;
  return *this;
}

VariableView VariableView::operator+=(const VariableConstView &other) const {
  return plus_equals(*this, other);
}

VariableView VariableView::operator-=(const VariableConstView &other) const {
  return minus_equals(*this, other);
}

VariableView VariableView::operator*=(const VariableConstView &other) const {
  return times_equals(*this, other);
}

VariableView VariableView::operator/=(const VariableConstView &other) const {
  return divide_equals(*this, other);
}

} // namespace scipp::variable
