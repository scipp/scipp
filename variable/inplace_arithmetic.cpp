// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arithmetic.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

// Note: These will broadcast/transpose the RHS if required. We do not support
// changing the dimensions of the LHS though!
Variable &Variable::operator+=(const VariableConstView &other) & {
  VariableView(*this) += other;
  return *this;
}

Variable &Variable::operator-=(const VariableConstView &other) & {
  VariableView(*this) -= other;
  return *this;
}

Variable &Variable::operator*=(const VariableConstView &other) & {
  VariableView(*this) *= other;
  return *this;
}

Variable &Variable::operator/=(const VariableConstView &other) & {
  VariableView(*this) /= other;
  return *this;
}

VariableView VariableView::operator+=(const VariableConstView &other) const {
  transform_in_place(*this, other, core::element::plus_equals);
  return *this;
}

VariableView VariableView::operator-=(const VariableConstView &other) const {
  transform_in_place(*this, other, core::element::minus_equals);
  return *this;
}

VariableView VariableView::operator*=(const VariableConstView &other) const {
  transform_in_place(*this, other, core::element::times_equals);
  return *this;
}

VariableView VariableView::operator/=(const VariableConstView &other) const {
  transform_in_place(*this, other, core::element::divide_equals);
  return *this;
}

} // namespace scipp::variable
