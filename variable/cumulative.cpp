// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/cumulative.h"
#include "scipp/core/element/cumulative.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/transform.h"

using namespace scipp;

namespace scipp::variable {

Variable cumsum(const VariableConstView &var, const Dim dim,
                const CumSumMode mode) {
  Variable cumulative = make_accumulant(var, dim, FillValue::ZeroNotBool);
  if (var.dims()[dim] == 0)
    return cumulative;
  Variable out(var);
  if (mode == CumSumMode::Inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan);
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan);
  return out;
}
Variable cumsum(const VariableConstView &var, const CumSumMode mode) {
  Variable cumulative(var, Dimensions{});
  Variable out(var);
  if (mode == CumSumMode::Inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan);
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan);
  return out;
}
Variable cumsum_bins(const VariableConstView &var, const CumSumMode mode) {
  Variable out(var);
  auto cumulative = Variable(variable::variableFactory().elem_dtype(var),
                             var.dims(), var.unit());
  if (mode == CumSumMode::Inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan);
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan);
  return out;
}

} // namespace scipp::variable
