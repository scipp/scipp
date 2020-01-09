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
  sin(out, out);
  return out;
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
  cos(out, out);
  return out;
}

Variable cos(Variable &&var) {
  using std::cos;
  Variable out(std::move(var));
  cos(out, out);
  return out;
}

VariableProxy cos(const VariableConstProxy &var, const VariableProxy &out) {
  using std::cos;
  out.assign(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  transform_in_place<double, float>(
      out, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](auto &x) { x = cos(x); }});
  return out;
}

Variable tan(const VariableConstProxy &var) {
  using std::tan;
  Variable out(var);
  tan(out, out);
  return out;
}

Variable tan(Variable &&var) {
  using std::tan;
  Variable out(std::move(var));
  tan(out, out);
  return out;
}

VariableProxy tan(const VariableConstProxy &var, const VariableProxy &out) {
  using std::tan;
  out.assign(var);
  if (var.unit() == units::deg)
    out *= deg_to_rad;
  transform_in_place<double, float>(
      out, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](auto &x) { x = tan(x); }});
  return out;
}

Variable asin(const VariableConstProxy &var) {
  using std::asin;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return asin(x); }});
}

Variable asin(Variable &&var) {
  using std::asin;
  Variable out(std::move(var));
  asin(out, out);
  return out;
}

VariableProxy asin(const VariableConstProxy &var, const VariableProxy &out) {
  using std::asin;
  transform_in_place<pair_self_t<double, float>>(
      out, var,
      overloaded{transform_flags::expect_no_variance_arg<0>,
                 transform_flags::expect_no_variance_arg<1>,
                 [](auto &x, const auto &y) { x = asin(y); }});
  return out;
}

Variable acos(const VariableConstProxy &var) {
  using std::acos;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return acos(x); }});
}

Variable acos(Variable &&var) {
  using std::acos;
  Variable out(std::move(var));
  acos(out, out);
  return out;
}

VariableProxy acos(const VariableConstProxy &var, const VariableProxy &out) {
  using std::acos;
  transform_in_place<pair_self_t<double, float>>(
      out, var,
      overloaded{transform_flags::expect_no_variance_arg<0>,
                 transform_flags::expect_no_variance_arg<1>,
                 [](auto &x, const auto &y) { x = acos(y); }});
  return out;
}

Variable atan(const VariableConstProxy &var) {
  using std::atan;
  return transform<double, float>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return atan(x); }});
}

Variable atan(Variable &&var) {
  using std::atan;
  Variable out(std::move(var));
  atan(out, out);
  return out;
}

VariableProxy atan(const VariableConstProxy &var, const VariableProxy &out) {
  using std::atan;
  transform_in_place<pair_self_t<double, float>>(
      out, var,
      overloaded{transform_flags::expect_no_variance_arg<0>,
                 transform_flags::expect_no_variance_arg<1>,
                 [](auto &x, const auto &y) { x = atan(y); }});
  return out;
}

} // namespace scipp::core
