// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
#ifndef SCIPP_CORE_COMPARISON_H
#define SCIPP_CORE_COMPARISON_H

#include "dtype.h"
#include "scipp/core/dataset.h"
#include "scipp/core/string.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include <atomic>

namespace scipp::core {

/// Tests if the unit, values (and variances where appropriate) of two
/// Variables are within an absolute tolerance.
template <typename T>
bool is_approx(const VariableConstView &a, const VariableConstView &b,
               const T tol) {
  if (a.dtype() != b.dtype()) {
    std::string message = "is_approx. Types do not match. dtype a " +
                          to_string(a.dtype()) + ". dtype b " +
                          to_string(b.dtype());
    throw except::TypeError(message);
  }
  if (dtype<T> != a.dtype()) {
    std::string message = "is_approx. Type of tol " + to_string(dtype<T>) +
                          " not same as type of input arguments " +
                          to_string(a.dtype());
    throw except::TypeError(message);
  }

  if (a.hasVariances() != b.hasVariances())
    return false;

  Variable aa(a);
  std::atomic_flag mismatch = ATOMIC_FLAG_INIT;
  transform_in_place<pair_self_t<T>>(
      aa, b,
      scipp::overloaded{
          [&](const auto &va, const auto &vb) {
            if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(va)>> !=
                          is_ValueAndVariance_v<std::decay_t<decltype(vb)>>) {
              mismatch.test_and_set();
            } else if constexpr (is_ValueAndVariance_v<
                                     std::decay_t<decltype(va)>>) {
              if ((std::abs(va.value - vb.value) >= tol) ||
                  (std::abs(va.variance - vb.variance) >= tol))
                mismatch.test_and_set();
            } else {
              if (std::abs(va - vb) >= tol)
                mismatch.test_and_set();
            }
          },
          [&](units::Unit &ua, const units::Unit &ub) {
            if (ua != ub)
              mismatch.test_and_set();
          }});
  return !mismatch.test_and_set();
}

} // namespace scipp::core

#endif // SCIPP_CORE_COMPARISON_H
