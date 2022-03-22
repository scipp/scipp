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

template <class T, class... Remaining>
std::optional<Variable>
try_isclose_spatial(const Variable &a, const Variable &b, const Variable &rtol,
                    const Variable &atol, const NanComparisons equal_nans) {
  if (a.dtype() == dtype<T>)
    return std::optional(
        all(isclose(a.elements<T>(), b.elements<T>(), rtol, atol, equal_nans),
            Dim::InternalStructureComponent));
  if constexpr (sizeof...(Remaining) > 0)
    return try_isclose_spatial<Remaining...>(a, b, rtol, atol, equal_nans);
  else
    return std::nullopt;
}
} // namespace

Variable isclose(const Variable &a, const Variable &b, const Variable &rtol,
                 const Variable &atol, const NanComparisons equal_nans) {
  core::expect::unit(rtol, scipp::units::dimensionless, " For rtol arg");
  if (const auto r =
          try_isclose_spatial<Eigen::Vector3d, Eigen::Matrix3d, Eigen::Affine3d,
                              core::Translation, core::Quaternion>(
              a, b, rtol, atol, equal_nans);
      r.has_value()) {
    return *r;
  }

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
