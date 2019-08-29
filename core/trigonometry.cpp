// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/core/except.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

namespace scipp::core {

struct fail_if_variance {
  template <class T>
  scipp::core::detail::ValueAndVariance<T>
  operator()(const scipp::core::detail::ValueAndVariance<T> &) const {
    // TODO A better way for disabling operations with variances is needed.
    throw std::runtime_error(
        "Inverse trigonometric operation requires dimensionless input.");
  }
};

Variable acos(const Variable &var) {
  using std::acos;
  return transform<double>(
      var,
      overloaded{fail_if_variance{}, [](const auto &x) { return acos(x); }});
}

} // namespace scipp::core
