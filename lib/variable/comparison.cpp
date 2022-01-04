// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Piotr Rozyczko
#include "scipp/core/element/comparison.h"
#include "scipp/core/eigen.h"
#include "scipp/units/string.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/math.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp::core;

namespace scipp::variable {

namespace {
Variable _values(Variable &&in) { return in.has_variances() ? values(in) : in; }
} // namespace

Variable isclose(const Variable &a, const Variable &b, const Variable &rtol,
                 const Variable &atol, const NanComparisons equal_nans) {
  core::expect::unit(rtol, scipp::units::dimensionless, " For rtol arg");
  // Element expansion comparison for vectors
  if (a.dtype() == dtype<Eigen::Vector3d>)
    return all(isclose(a.elements<Eigen::Vector3d>(),
                       b.elements<Eigen::Vector3d>(), rtol, atol, equal_nans),
               Dim::InternalStructureComponent);

  auto tol = atol + rtol * abs(b);
  if (a.has_variances() && b.has_variances()) {
    return isclose(values(a), values(b), rtol, atol, equal_nans) &
           isclose(stddevs(a), stddevs(b), rtol, atol, equal_nans);
  } else {
    if (equal_nans == NanComparisons::Equal)
      return variable::transform(a, b, _values(std::move(tol)),
                                 element::isclose_equal_nan, "isclose");
    else
      return variable::transform(a, b, _values(std::move(tol)),
                                 element::isclose, "isclose");
  }
}

} // namespace scipp::variable
