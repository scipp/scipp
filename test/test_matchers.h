// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#pragma once

#include <gmock/gmock-matchers.h>

#include "scipp/common/traits.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"
#include "scipp/units/unit.h"
#include "scipp/variable/variable.h"

namespace scipp::testing {
/// Return a suitable absolute tolerance for comparing elements of the given
/// dtype.
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

/// Return a DataArray with every bin sorted according to its coordinates.
dataset::DataArray sort_bins(dataset::DataArray data);

MATCHER_P2(ScippNear, expected, tolerance, "") {
  return all(is_approx(arg, expected,
                       tol_or_default(tolerance, expected.dtype()) *
                           units::one))
      .template value<bool>();
}

MATCHER_P(EqDisorder, expected, "") {
  return ::testing::ExplainMatchResult(::testing::Eq(sort_bins(expected)),
                                       sort_bins(arg), result_listener);
}
} // namespace scipp::testing