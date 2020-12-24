// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/math.h"
#include "scipp/core/transform_common.h"
#include "scipp/variable/math.h"
#include "scipp/variable/transform.h"
#include <scipp/core/element/arithmetic.h>

using namespace scipp::core;

namespace scipp::variable {

Variable abs(Variable &&var) {
  abs(var, var);
  return std::move(var);
}

Variable dot(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::dot);
}

Variable norm(const VariableConstView &var) {
  return transform(var, element::norm);
}

Variable sqrt(Variable &&var) {
  sqrt(var, var);
  return std::move(var);
}

Variable reciprocal(Variable &&var) {
  auto out(std::move(var));
  reciprocal(out, out);
  return out;
}

Variable floor_div(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::floor_div);
}

} // namespace scipp::variable
