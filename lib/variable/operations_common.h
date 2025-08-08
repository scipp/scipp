// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/flags.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/reciprocal.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

// Helpers for in-place reductions and reductions with groupby.
SCIPP_VARIABLE_EXPORT Variable mean_impl(const Variable &var, const Dim dim,
                                         const Variable &masks_sum);
SCIPP_VARIABLE_EXPORT Variable nanmean_impl(const Variable &var, const Dim dim,
                                            const Variable &masks_sum);

template <class T> T normalize_impl(const T &numerator, T denominator) {
  // Numerator may be an int or a Eigen::Vector3d => use double
  // This approach would be wrong if we supported vectors of float
  const auto type =
      numerator.dtype() == dtype<float> ? dtype<float> : dtype<double>;
  denominator.setUnit(sc_units::one);
  return numerator *
         reciprocal(astype(denominator, type, CopyPolicy::TryAvoid));
}

SCIPP_VARIABLE_EXPORT void expect_valid_bin_indices(const Variable &indices,
                                                    const Dim dim,
                                                    const Sizes &buffer_sizes);

template <class T>
Variable make_bins_impl(Variable indices, const Dim dim, T &&buffer);

template <class T, class Op> auto reduce_all_dims(const T &obj, const Op &op) {
  if (obj.dims().empty()) {
    if (is_bins(obj))
      return op(obj, Dim::Invalid);
    else
      return copy(obj);
  }
  auto out = op(obj, obj.dims().inner());
  while (!out.dims().empty())
    out = op(out, out.dims().inner());
  return out;
}

} // namespace scipp::variable
