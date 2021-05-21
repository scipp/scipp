// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

// Helpers for in-place reductions and reductions with groupby.
SCIPP_VARIABLE_EXPORT void sum_impl(Variable &summed, const Variable &var);
SCIPP_VARIABLE_EXPORT void all_impl(Variable &out, const Variable &var);
SCIPP_VARIABLE_EXPORT void any_impl(Variable &out, const Variable &var);
SCIPP_VARIABLE_EXPORT void max_impl(Variable &out, const Variable &var);
SCIPP_VARIABLE_EXPORT void min_impl(Variable &out, const Variable &var);
SCIPP_VARIABLE_EXPORT Variable mean_impl(const Variable &var, const Dim dim,
                                         const Variable &masks_sum);
SCIPP_VARIABLE_EXPORT Variable &mean_impl(const Variable &var, const Dim dim,
                                          const Variable &masks_sum,
                                          Variable &out);
SCIPP_VARIABLE_EXPORT Variable nanmean_impl(const Variable &var, const Dim dim,
                                            const Variable &masks_sum);
SCIPP_VARIABLE_EXPORT Variable &nanmean_impl(const Variable &var, const Dim dim,
                                             const Variable &masks_sum,
                                             Variable &out);

template <class T> T normalize_impl(const T &nominator, const T &denominator) {
  // Nominator may be and int or a Eigen::Vector3d => use double
  // This approach would be wrong if we supported vectors of float
  const auto type =
      nominator.dtype() == dtype<float> ? dtype<float> : dtype<double>;
  return nominator * reciprocal(astype(denominator, type));
}

} // namespace scipp::variable
