// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/variable/variable.h"

namespace scipp::variable {

// Helpers for in-place reductions and reductions with groupby.
void flatten_impl(const VariableView &summed, const VariableConstView &var,
                  const VariableConstView &mask);
void sum_impl(const VariableView &summed, const VariableConstView &var);
void all_impl(const VariableView &out, const VariableConstView &var);
void any_impl(const VariableView &out, const VariableConstView &var);
void max_impl(const VariableView &out, const VariableConstView &var);
void min_impl(const VariableView &out, const VariableConstView &var);
SCIPP_VARIABLE_EXPORT Variable mean_impl(const VariableConstView &var,
                                         const Dim dim,
                                         const VariableConstView &masks_sum);
SCIPP_VARIABLE_EXPORT VariableView mean_impl(const VariableConstView &var,
                                             const Dim dim,
                                             const VariableConstView &masks_sum,
                                             const VariableView &out);

template <class Op>
Variable reduce_all_dims(const VariableConstView &var, const Op &op) {
  if (var.dims().empty())
    return Variable(var);
  Variable out = op(var, var.dims().inner());
  while (!out.dims().empty())
    out = op(out, out.dims().inner());
  return out;
}

} // namespace scipp::variable
