// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/common/constants.h"
#include "scipp/core/element/trigonometry.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/trigonometry.h"

using namespace scipp::core;

namespace scipp::variable {

const auto deg_to_rad = makeVariable<double>(
    Dims(), Shape(), units::rad / units::deg, Values{pi<double> / 180.0});

Variable sin(const VariableConstView &var) {
  Variable out(var);
  sin(out, out);
  return out;
}

Variable sin(Variable &&var) {
  Variable out(std::move(var));
  sin(out, out);
  return out;
}

VariableView sin(const VariableConstView &var, const VariableView &out) {
  core::expect::unit_any_of(var, {units::rad, units::deg});
  out.assign(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  transform_in_place(out, out, element::sin_out_arg);
  return out;
}

Variable cos(const VariableConstView &var) {
  Variable out(var);
  cos(out, out);
  return out;
}

Variable cos(Variable &&var) {
  Variable out(std::move(var));
  cos(out, out);
  return out;
}

VariableView cos(const VariableConstView &var, const VariableView &out) {
  core::expect::unit_any_of(var, {units::rad, units::deg});
  out.assign(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  transform_in_place(out, out, element::cos_out_arg);
  return out;
}

Variable tan(const VariableConstView &var) {
  Variable out(var);
  tan(out, out);
  return out;
}

Variable tan(Variable &&var) {
  Variable out(std::move(var));
  tan(out, out);
  return out;
}

VariableView tan(const VariableConstView &var, const VariableView &out) {
  core::expect::unit_any_of(var, {units::rad, units::deg});
  out.assign(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  transform_in_place(out, out, element::tan_out_arg);
  return out;
}

Variable asin(const VariableConstView &var) {
  return transform(var, element::asin);
}

Variable asin(Variable &&var) {
  Variable out(std::move(var));
  asin(out, out);
  return out;
}

VariableView asin(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::asin_out_arg);
  return out;
}

Variable acos(const VariableConstView &var) {
  return transform(var, element::acos);
}

Variable acos(Variable &&var) {
  Variable out(std::move(var));
  acos(out, out);
  return out;
}

VariableView acos(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::acos_out_arg);
  return out;
}

Variable atan(const VariableConstView &var) {
  return transform(var, element::atan);
}

Variable atan(Variable &&var) {
  Variable out(std::move(var));
  atan(out, out);
  return out;
}

VariableView atan(const VariableConstView &var, const VariableView &out) {
  transform_in_place(out, var, element::atan_out_arg);
  return out;
}

Variable atan2(const VariableConstView &y, const VariableConstView &x) {
  return transform(y, x, element::atan2);
}

VariableView atan2(const VariableConstView &y, const VariableConstView &x,
                   const VariableView &out) {
  transform_in_place(out, y, x, element::atan2_out_arg);
  return out;
}

} // namespace scipp::variable
