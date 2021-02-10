// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

} // namespace scipp::variable
