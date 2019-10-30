// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#ifndef SCIPP_CORE_COMPARISON_H
#define SCIPP_CORE_COMPARISON_H

#include "scipp/core/dataset.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include <atomic>

namespace scipp::core {

/**
 * Tests if the unit, values (and variances where appropriate) of two Variables
 * are within an absolute tolerance.
 */
template <typename T>
bool is_approx(Variable a, const Variable &b, const T tol) {
  if (a.dtype() != b.dtype())
    return false;

  std::atomic<bool> match{true};
  transform_in_place<pair_self_t<T>>(
      a, b,
      scipp::overloaded{
          [&](const T va, const T vb) {
            match.store(match.load() && std::abs(va - vb) < tol);
          },
          [&](const T va, const auto &vb) {
            match.store(match.load() && std::abs(va - vb.value) < tol);
          },
          [&](const auto &va, const T vb) {
            match.store(match.load() && std::abs(va.value - vb) < tol);
          },
          [&](const auto &va, const auto &vb) {
            match.store(match.load() &&
                        ((std::abs(va.value - vb.value) < tol) &&
                         (std::abs(va.variance - vb.variance) < tol)));
          },
          [&](units::Unit &ua, const units::Unit &ub) {
            match.store(match.load() && ua == ub);
          }});
  return match;
}

} // namespace scipp::core

#endif // SCIPP_CORE_COMPARISON_H
