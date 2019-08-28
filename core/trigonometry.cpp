// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

namespace scipp::core {

constexpr auto inverse_trigonometric = overloaded{
    [](const scipp::core::detail::ValueAndVariance<double>)
        -> scipp::core::detail::ValueAndVariance<double> {
      // TODO A better way for disabling operations with variances is needed.
      throw std::runtime_error(
          "Inverse trigonometric operation requires dimensionless input.");
    },
    [](const units::Unit &u) {
      expect::equals(u, units::Unit(units::dimensionless));
      return units::rad;
    }};

Variable acos(const Variable &var) {
  using std::acos;
  return transform<double>(
      var,
      overloaded{inverse_trigonometric, [](const auto &x) { return acos(x); }});
}

} // namespace scipp::core
