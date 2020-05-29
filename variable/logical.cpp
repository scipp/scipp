// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/logical.h"
#include "scipp/variable/logical.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable &Variable::operator|=(const VariableConstView &other) & {
  VariableView(*this) |= other;
  return *this;
}

Variable &Variable::operator&=(const VariableConstView &other) & {
  VariableView(*this) &= other;
  return *this;
}

Variable &Variable::operator^=(const VariableConstView &other) & {
  VariableView(*this) ^= other;
  return *this;
}

VariableView VariableView::operator|=(const VariableConstView &other) const {
  transform_in_place(*this, other, element::logical_or_equals);
  return *this;
}

VariableView VariableView::operator&=(const VariableConstView &other) const {
  transform_in_place(*this, other, element::logical_and_equals);
  return *this;
}

VariableView VariableView::operator^=(const VariableConstView &other) const {
  transform_in_place(*this, other, element::logical_xor_equals);
  return *this;
}

Variable operator&(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::logical_and);
}
Variable operator|(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::logical_or);
}
Variable operator^(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::logical_xor);
}

Variable Variable::operator~() const {
  return transform(*this, core::element::logical_not);
}

} // namespace scipp::variable
