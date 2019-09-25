// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)

#ifndef SCIPP_COMPARISON_H
#define SCIPP_COMPARISON_H

#include "../operators.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

namespace scipp::core {

template <typename T>
bool approx_same(const Variable &a, const Variable &b, const T &tolerance) {
  const auto abs_tolerance = abs(tolerance);
  auto in_tolerance = transform<pair_self_t<double>>(
      a, b,
      scipp::overloaded{[abs_tolerance](const auto &u, const auto &v) {
                          return abs(u - v) < abs_tolerance;
                        },
                        [](const units::Unit &, const units::Unit &) {
                          return units::dimensionless;
                        }});
  return std::all_of(in_tolerance.template values<bool>().begin(),
                     in_tolerance.template values<bool>().end(),
                     [](const auto &val) { return val; });
}

} // namespace scipp::core

#endif // SCIPP_COMPARISON_H
