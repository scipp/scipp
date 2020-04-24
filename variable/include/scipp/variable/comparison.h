// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#pragma once

#include "scipp/core/dtype.h"
#include "scipp/variable/string.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

#include <atomic>

namespace scipp::variable {

/// Functions to provide numpy-like element-wise comparison
/// between two Variables.

SCIPP_VARIABLE_EXPORT Variable less(const VariableConstView &x,
                                    const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable greater(const VariableConstView &x,
                                       const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable greater_equal(const VariableConstView &x,
                                             const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable less_equal(const VariableConstView &x,
                                          const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable equal(const VariableConstView &x,
                                     const VariableConstView &y);
SCIPP_VARIABLE_EXPORT Variable not_equal(const VariableConstView &x,
                                         const VariableConstView &y);

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
  if (dtype<T> != a.dtype() && dtype<event_list<T>> != a.dtype()) {
    std::string message = "is_approx. Type of tol " + to_string(dtype<T>) +
                          " not same as type of input arguments " +
                          to_string(a.dtype());
    throw except::TypeError(message);
  }

  if (a.hasVariances() != b.hasVariances())
    return false;

  Variable aa(a);
  std::atomic_flag mismatch = ATOMIC_FLAG_INIT;
  transform_in_place<core::pair_self_t<T>>(
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

} // namespace scipp::variable
