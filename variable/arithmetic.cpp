// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arithmetic.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable Variable::operator-() const {
  return transform(*this, element::unary_minus);
}

Variable VariableConstView::operator-() const {
  Variable copy(*this);
  return -copy;
}

Variable operator+(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::plus);
}
Variable operator-(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::minus);
}
Variable operator*(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::times);
}
Variable operator/(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::divide);
}

} // namespace scipp::variable
