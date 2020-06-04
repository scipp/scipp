// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/math.h"
#include "scipp/variable/math.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable abs(const VariableConstView &var) {
  return transform<double, float>(var, element::abs);
}

Variable abs(Variable &&var) {
  abs(var, var);
  return std::move(var);
}

VariableView abs(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::abs_out_arg);
  return out;
}

Variable dot(const VariableConstView &a, const VariableConstView &b) {
  return transform(a, b, element::dot);
}

Variable norm(const VariableConstView &var) {
  return transform(var, element::norm);
}

Variable sqrt(const VariableConstView &var) {
  return transform<double, float>(var, element::sqrt);
}

Variable sqrt(Variable &&var) {
  sqrt(var, var);
  return std::move(var);
}

VariableView sqrt(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::sqrt_out_arg);
  return out;
}

Variable reciprocal(const VariableConstView &var) {
  return transform(var, element::reciprocal);
}

Variable reciprocal(Variable &&var) {
  auto out(std::move(var));
  reciprocal(out, out);
  return out;
}

VariableView reciprocal(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::reciprocal_out_arg);
  return out;
}

} // namespace scipp::variable
