// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include "scipp/core/element/comparison.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/math.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {

Variable isclose(const VariableConstView &a, const VariableConstView &b,
                 const VariableConstView rtol, const VariableConstView atol,
                 const NanComparisons equal_nans) {
  const auto tol = atol + rtol * abs(b);
  if (a.hasVariances() && b.hasVariances()) {
    const auto error_tol = atol + rtol * abs(variances(b));
    if (equal_nans == NanComparisons::Equal)
      return variable::transform(
                 a, b, error_tol.hasVariances() ? values(error_tol) : error_tol,
                 element::isclose_equal_nan) &
             variable::transform(sqrt(variances(a)), sqrt(variances(b)),
                                 error_tol.hasVariances() ? values(error_tol)
                                                          : error_tol,
                                 element::isclose_equal_nan);
    else
      return variable::transform(
                 a, b, error_tol.hasVariances() ? values(error_tol) : error_tol,
                 element::isclose) &
             variable::transform(sqrt(variances(a)), sqrt(variances(b)),
                                 error_tol.hasVariances() ? values(error_tol)
                                                          : error_tol,
                                 element::isclose);
  } else {
    if (equal_nans == NanComparisons::Equal)
      return variable::transform(a, b, tol.hasVariances() ? values(tol) : tol,
                                 element::isclose_equal_nan);
    else
      return variable::transform(a, b, tol.hasVariances() ? values(tol) : tol,
                                 element::isclose);
  }
}

} // namespace scipp::variable
