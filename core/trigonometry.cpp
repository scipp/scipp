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

struct no_variance {
  template <class T>
  scipp::core::detail::ValueAndVariance<T>
  operator()(const scipp::core::detail::ValueAndVariance<T> &) const {
    // TODO A better way for disabling operations with variances is needed.
    throw std::runtime_error("Operation does not support data with variances.");
  }
};

Variable to_rad(Variable var) {
  const auto scale =
      makeVariable<double>({}, units::rad / units::deg, {pi<double> / 180.0});
  return var * scale;
}

Variable sin(const Variable &var) {
  using std::sin;
  const Variable &rad = var.unit() == units::deg ? to_rad(var) : var;
  return transform<double, float>(
      rad, overloaded{no_variance{}, [](const auto &x) { return sin(x); }});
}

Variable cos(const Variable &var) {
  using std::cos;
  const Variable &rad = var.unit() == units::deg ? to_rad(var) : var;
  return transform<double, float>(
      rad, overloaded{no_variance{}, [](const auto &x) { return cos(x); }});
}

Variable tan(const Variable &var) {
  using std::tan;
  const Variable &rad = var.unit() == units::deg ? to_rad(var) : var;
  return transform<double, float>(
      rad, overloaded{no_variance{}, [](const auto &x) { return tan(x); }});
}

Variable asin(const Variable &var) {
  using std::asin;
  return transform<double, float>(
      var, overloaded{no_variance{}, [](const auto &x) { return asin(x); }});
}

Variable acos(const Variable &var) {
  using std::acos;
  return transform<double, float>(
      var, overloaded{no_variance{}, [](const auto &x) { return acos(x); }});
}

Variable atan(const Variable &var) {
  using std::atan;
  return transform<double, float>(
      var, overloaded{no_variance{}, [](const auto &x) { return atan(x); }});
}

} // namespace scipp::core
