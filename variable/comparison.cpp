// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include "scipp/core/element/comparison.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {

Variable is_approx(const VariableConstView &a, const VariableConstView &b,
                   const VariableConstView &t) {
  return transform(a, b, t, element::is_approx);
}

} // namespace scipp::variable
