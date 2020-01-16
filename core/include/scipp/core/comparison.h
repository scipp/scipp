// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
#ifndef SCIPP_CORE_COMPARISON_H
#define SCIPP_CORE_COMPARISON_H

#include "dtype.h"
#include "scipp/core/dataset.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"
#include "scipp/core/string.h"

#include <atomic>

namespace scipp::core {

/// Tests if the unit, values (and variances where appropriate) of two
/// Variables are within an absolute tolerance.
template <typename T>
bool is_approx(const VariableConstProxy &a, const VariableConstProxy &b,
               const T tol) {
  if (a.dtype() != b.dtype())
    return false; // TODO this should throw rather than return false IMHO

  if(dtype<T> != a.dtype()) {
    throw std::runtime_error("Type of tolerance " + to_string(dtype<T>) + " not same as lh operand " + to_string(a.dtype()));
  }
  if(dtype<T> != a.dtype()) {
    throw std::runtime_error("Type of tolerance " + to_string(dtype<T>) + " not same as rh operand " + to_string(a.dtype()));
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
