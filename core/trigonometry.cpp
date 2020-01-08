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

const auto deg_to_rad =
    makeVariable<double>(Dims(), Shape(), units::Unit(units::rad / units::deg),
                         Values{pi<double> / 180.0});

Variable sin(const VariableConstProxy &var) {
  using std::sin;
  Variable out(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  return transform<double, float>(
      out, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return sin(x); }});
}

Variable sin(Variable &&var) {
  using std::sin;
  Variable out(std::move(var));
  sin(out, out);
  return out;
}

VariableProxy sin(const VariableConstProxy &var, const VariableProxy &out) {
  using std::sin;
  out.assign(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  transform_in_place<double, float>(
      out, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](auto &x) { x = sin(x); }});
  return out;
}

Variable cos(const VariableConstProxy &var) {
  using std::cos;
  Variable out(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  return transform<double, float>(
      out, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return cos(x); }});
}

Variable tan(const VariableConstProxy &var) {
  using std::tan;
  Variable out(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  return transform<double, float>(
      out, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return tan(x); }});
}

Variable asin(const VariableConstProxy &var) {
  using std::asin;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return asin(x); }});
}

Variable acos(const VariableConstProxy &var) {
  using std::acos;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return acos(x); }});
}

Variable atan(const VariableConstProxy &var) {
  using std::atan;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return atan(x); }});
}

} // namespace scipp::core
