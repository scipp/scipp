// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#pragma once

#include <gmock/gmock-matchers.h>

#include "scipp/variable/variable.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"

namespace scipp::testing {
/// Return a suitable absolute tolerance for comparing elements of the given dtype.
constexpr double abs_tolerance_for(const scipp::core::DType dtype) {
  if (dtype == scipp::core::dtype<float>) {
    return 1e-5f;
  }
  if (dtype == scipp::core::dtype<double>) {
    return 1e-14;
  }
  return 0;
}

/// Return the given tolerance or the default value for a dtype.
constexpr double tol_or_default(const std::optional<double> tol,
                                const scipp::core::DType dtype) {
  return tol.value_or(abs_tolerance_for(dtype));
}

// TODO variances
MATCHER_P2(IsNearVariable, expected, tol,
           "Scipp Variables are approximately equal") {
  return all(is_approx(arg, expected, tol * units::one)).template value<bool>();
}

MATCHER_P2(DataIsNear, expected, tol, "") {
  return all(is_approx(arg.data(), expected.data(), tol * units::one))
      .template value<bool>();
}

MATCHER_P(CoordsIsEqual, expected, "") {
  return arg.coords() == expected.coords();
}

MATCHER_P(AttrsIsEqual, expected, "") {
  return arg.attrs() == expected.attrs();
}

MATCHER_P(MasksIsEqual, expected, "") {
  return arg.masks() == expected.masks();
}

// TODO variances
MATCHER_P2(IsNearDataArray, expected, tol,
           "Scipp DataArrays are approximately equal") {
  return ::testing::ExplainMatchResult(
      ::testing::AllOf(DataIsNear(expected, tol), CoordsIsEqual(expected),
                       AttrsIsEqual(expected), MasksIsEqual(expected)),
      arg, result_listener);
}

DataArray sort_bins(DataArray data);

// TODO name
MATCHER_P2(IsNearScipp, expected, tolerance, "") {
  const double tol = tol_or_default(tolerance, expected.dtype());
  if constexpr (std::is_convertible_v<decltype(expected),
                                      variable::VariableConstView>) {
    return ::testing::ExplainMatchResult(IsNearVariable(expected, tol), arg,
                                         result_listener);
  } else if constexpr (std::is_convertible_v<decltype(expected),
                                             dataset::DataArrayConstView>) {
    if (is_bins(expected)) {
      return bins_equal(arg, expected, tol);
    } else {
      return ::testing::ExplainMatchResult(IsNearDataArray(expected, tol), arg,
                                           result_listener);
    }
  } else {
    static_assert(common::always_false<decltype(expected)>, "Unsupported type");
  }
}

MATCHER_P(EqDisorder, expected, "") {
  return ::testing::ExplainMatchResult(::testing::Eq(sort_bins(expected)),
                                       sort_bins(arg), result_listener);
}
}