// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/cumulative.h"
#include "scipp/core/element/cumulative.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

using namespace scipp;

namespace scipp::variable {

Variable cumsum(const VariableConstView &var, const Dim dim,
                const bool inclusive) {
  if (var.dims()[dim] == 0)
    return Variable{var};
  Variable cumulative(var.slice({dim, 0}));
  fill_zeros(cumulative);
  Variable out(var);
  if (inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan);
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan);
  return out;
}
Variable cumsum(const VariableConstView &var, const bool inclusive) {
  Variable cumulative(var, Dimensions{});
  Variable out(var);
  if (inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan);
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan);
  return out;
}
Variable cumsum_bins(const VariableConstView &var, const bool inclusive) {
  Variable out(var);
  auto cumulative = Variable(variable::variableFactory().elem_dtype(var),
                             var.dims(), var.unit());
  if (inclusive)
    accumulate_in_place(cumulative, out, core::element::inclusive_scan);
  else
    accumulate_in_place(cumulative, out, core::element::exclusive_scan);
  return out;
}

} // namespace scipp::variable
