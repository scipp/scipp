// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/common/constants.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

namespace scipp::core {

Variable to_rad(Variable var) {
  const auto scale =
      createVariable<double>(Dims(), Shape(), units::Unit(units::rad / units::deg), Values{pi<double> / 180.0});
  return var * scale;
}

Variable sin(const Variable &var) {
  using std::sin;
  const Variable &rad = var.unit() == units::deg ? to_rad(var) : var;
  return transform<double, float>(
      rad, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return sin(x); }});
}

Variable cos(const Variable &var) {
  using std::cos;
  const Variable &rad = var.unit() == units::deg ? to_rad(var) : var;
  return transform<double, float>(
      rad, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return cos(x); }});
}

Variable tan(const Variable &var) {
  using std::tan;
  const Variable &rad = var.unit() == units::deg ? to_rad(var) : var;
  return transform<double, float>(
      rad, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return tan(x); }});
}

Variable asin(const Variable &var) {
  using std::asin;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return asin(x); }});
}

Variable acos(const Variable &var) {
  using std::acos;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return acos(x); }});
}

Variable atan(const Variable &var) {
  using std::atan;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return atan(x); }});
}

} // namespace scipp::core
