// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/element/trigonometry.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/trigonometry.h"
#include "scipp/variable/variable_factory.h"

using namespace scipp::core;

namespace scipp::variable {

Variable sin(const Variable &var) {
  auto out = variableFactory().empty_like(var, std::nullopt);
  sin(var, out);
  return out;
}

Variable &sin(const Variable &var, Variable &out) {
  transform_in_place(out, to_unit(var, units::rad), element::sin_out_arg,
                     "sin");
  return out;
}

Variable cos(const Variable &var) {
  auto out = variableFactory().empty_like(var, std::nullopt);
  cos(var, out);
  return out;
}

Variable &cos(const Variable &var, Variable &out) {
  transform_in_place(out, to_unit(var, units::rad), element::cos_out_arg,
                     "cos");
  return out;
}

Variable tan(const Variable &var) {
  auto out = variableFactory().empty_like(var, std::nullopt);
  tan(var, out);
  return out;
}

Variable &tan(const Variable &var, Variable &out) {
  transform_in_place(out, to_unit(var, units::rad), element::tan_out_arg,
                     "tan");
  return out;
}

Variable atan2(const Variable &y, const Variable &x) {
  return transform(y, x, element::atan2, "atan2");
}

Variable &atan2(const Variable &y, const Variable &x, Variable &out) {
  transform_in_place(out, y, x, element::atan2_out_arg, "atan2");
  return out;
}

} // namespace scipp::variable
