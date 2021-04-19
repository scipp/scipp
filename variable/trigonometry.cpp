// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/element/trigonometry.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/trigonometry.h"

using namespace scipp::core;

namespace scipp::variable {

Variable sin(const Variable &var) {
  auto out = empty(var.dims(), units::one, var.dtype());
  sin(var, out);
  return out;
}

Variable sin(Variable &&var) {
  Variable out(std::move(var));
  sin(out, out);
  return out;
}

Variable &sin(const Variable &var, Variable &out) {
  core::expect::unit_any_of(var, {units::rad, units::deg});
  transform_in_place(out, to_unit(var, units::rad), element::sin_out_arg);
  return out;
}

Variable cos(const Variable &var) {
  auto out = empty(var.dims(), units::one, var.dtype());
  cos(var, out);
  return out;
}

Variable cos(Variable &&var) {
  Variable out(std::move(var));
  cos(out, out);
  return out;
}

Variable &cos(const Variable &var, Variable &out) {
  core::expect::unit_any_of(var, {units::rad, units::deg});
  transform_in_place(out, to_unit(var, units::rad), element::cos_out_arg);
  return out;
}

Variable tan(const Variable &var) {
  auto out = empty(var.dims(), units::one, var.dtype());
  tan(var, out);
  return out;
}

Variable tan(Variable &&var) {
  Variable out(std::move(var));
  tan(out, out);
  return out;
}

Variable &tan(const Variable &var, Variable &out) {
  core::expect::unit_any_of(var, {units::rad, units::deg});
  transform_in_place(out, to_unit(var, units::rad), element::tan_out_arg);
  return out;
}

Variable asin(const Variable &var) { return transform(var, element::asin); }

Variable asin(Variable &&var) {
  Variable out(std::move(var));
  asin(out, out);
  return out;
}

Variable &asin(const Variable &var, Variable &out) {
  transform_in_place(out, var, element::asin_out_arg);
  return out;
}

Variable acos(const Variable &var) { return transform(var, element::acos); }

Variable acos(Variable &&var) {
  Variable out(std::move(var));
  acos(out, out);
  return out;
}

Variable &acos(const Variable &var, Variable &out) {
  transform_in_place(out, var, element::acos_out_arg);
  return out;
}

Variable atan(const Variable &var) { return transform(var, element::atan); }

Variable atan(Variable &&var) {
  Variable out(std::move(var));
  atan(out, out);
  return out;
}

Variable &atan(const Variable &var, Variable &out) {
  transform_in_place(out, var, element::atan_out_arg);
  return out;
}

Variable atan2(const Variable &y, const Variable &x) {
  return transform(y, x, element::atan2);
}

Variable &atan2(const Variable &y, const Variable &x, Variable &out) {
  transform_in_place(out, y, x, element::atan2_out_arg);
  return out;
}

} // namespace scipp::variable
