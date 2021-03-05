// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include "scipp/core/element/comparison.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/math.h"
#include "scipp/variable/special_values.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {

Variable is_close(const VariableConstView &a, const VariableConstView &b,
                  const VariableConstView rtol, const VariableConstView atol,
                  const NanComparisons equal_nans) {
  const auto tol = atol + rtol * abs(b);
  if (equal_nans == NanComparisons::Equal)
    return variable::transform(a, b, tol, element::is_approx_equal_nan);
  else
    return variable::transform(a, b, tol, element::is_approx);
}

} // namespace scipp::variable
