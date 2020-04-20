// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include <cmath>

#include "scipp/core/operators.h"
#include "scipp/core/element_geometric_operations.h"
#include "scipp/core/except.h"
#include "scipp/core/dtype.h"
#include "scipp/variable/string.h"

#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_comparison.h"

using namespace scipp::core;

namespace scipp::variable {


Variable is_less(const VariableConstView &x, const VariableConstView &y) {
  if (x.dtype() != y.dtype())
    throw std::runtime_error(
        "Cannot compare Variables: Data types do not match.");
  if (x.unit() != y.unit())
    throw std::runtime_error(
        "Cannot compare Variables: Units do not match.");
  if (x.hasVariances() || y.hasVariances())
    throw std::runtime_error(
        "Cannot compare Variables with variances.");
  return transform(x, y, element::less);
}

} // namespace scipp::variable

