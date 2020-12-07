// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/math.h"
#include "scipp/core/transform_common.h"
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
  transform_in_place(out, var, assign_unary{element::abs});
  return out;
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

Variable reciprocal(const VariableConstView &var) {
  return transform(var, element::reciprocal);
}

Variable reciprocal(Variable &&var) {
  auto out(std::move(var));
  reciprocal(out, out);
  return out;
}

VariableView reciprocal(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, assign_unary{element::reciprocal});
  return out;
}

Variable exp(const VariableConstView &var) {
  return transform(var, element::exp);
}

VariableView exp(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, assign_unary{element::exp});
  return out;
}

Variable log(const VariableConstView &var) {
  return transform(var, element::log);
}

VariableView log(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, assign_unary{element::log});
  return out;
}

Variable log10(const VariableConstView &var) {
  return transform(var, element::log10);
}

VariableView log10(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, assign_unary{element::log10});
  return out;
}

} // namespace scipp::variable
