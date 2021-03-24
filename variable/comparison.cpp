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

namespace {
Variable _values(Variable &&in) { return in.hasVariances() ? values(in) : in; }
} // namespace

Variable isclose(const Variable &a, const Variable &b, const Variable rtol,
                 const Variable atol, const NanComparisons equal_nans) {
  auto tol = atol + rtol * abs(b);
  if (a.hasVariances() && b.hasVariances()) {
    return isclose(values(a), values(b), rtol, atol, equal_nans) &
           isclose(stddevs(a), stddevs(b), rtol, atol, equal_nans);
  } else {
    if (equal_nans == NanComparisons::Equal)
      return variable::transform(a, b, _values(std::move(tol)),
                                 element::isclose_equal_nan);
    else
      return variable::transform(a, b, _values(std::move(tol)),
                                 element::isclose);
  }
}

} // namespace scipp::variable
