// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include <cmath>

#include "scipp/core/operators.h"
#include "scipp/core/element_geometric_operations.h"
#include "scipp/core/except.h"
#include "scipp/core/dtype.h"
#include "scipp/variable/string.h"

#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_comparison.h"

using namespace scipp::core;

namespace scipp::variable {

void check_comparability(const VariableConstView& x,
    const VariableConstView& y) {
    if (x.dtype() != y.dtype())
      throw std::runtime_error(
          "Cannot compare Variables: Data types do not match.");
    if (x.unit() != y.unit())
      throw std::runtime_error("Cannot compare Variables: Units do not match.");
    if (x.hasVariances() || y.hasVariances())
      throw std::runtime_error("Cannot compare Variables with variances.");
 }

Variable is_less(const VariableConstView &x, const VariableConstView &y) {
   check_comparability(x, y);
   return transform(x, y, element::less);
}

Variable is_greater(const VariableConstView &x, const VariableConstView &y) {
  check_comparability(x, y);
  return transform(x, y, element::greater);
}
Variable is_less_equal(const VariableConstView &x, const VariableConstView &y) {
  check_comparability(x, y);
  return transform(x, y, element::less_equal);
}
Variable is_greater_equal(const VariableConstView &x, const VariableConstView &y) {
  check_comparability(x, y);
  return transform(x, y, element::greater_equal);
}
Variable is_equal(const VariableConstView &x, const VariableConstView &y) {
  check_comparability(x, y);
  return transform(x, y, element::equal);
}
Variable is_not_equal(const VariableConstView &x, const VariableConstView &y) {
  check_comparability(x, y);
  return transform(x, y, element::not_equal);
}

} // namespace scipp::variable

