// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include <cmath>

#include "scipp/core/dtype.h"
#include "scipp/core/element/logical_operations.h"
#include "scipp/core/except.h"
#include "scipp/core/operators.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/string.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {

Variable less(const VariableConstView &x, const VariableConstView &y) {
  return transform(x, y, element::less);
}

Variable greater(const VariableConstView &x, const VariableConstView &y) {
  return transform(x, y, element::greater);
}
Variable less_equal(const VariableConstView &x, const VariableConstView &y) {
  return transform(x, y, element::less_equal);
}
Variable greater_equal(const VariableConstView &x, const VariableConstView &y) {
  return transform(x, y, element::greater_equal);
}
Variable equal(const VariableConstView &x, const VariableConstView &y) {
  return transform(x, y, element::equal);
}
Variable not_equal(const VariableConstView &x, const VariableConstView &y) {
  return transform(x, y, element::not_equal);
}

} // namespace scipp::variable
