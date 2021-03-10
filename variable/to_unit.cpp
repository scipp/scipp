// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/to_unit.h"

namespace scipp::variable {

Variable to_unit(const VariableConstView &var, const units::Unit &unit) {
  if (var.dtype() != dtype<double> && var.dtype() != dtype<float>)
    throw except::TypeError(
        "to_unit only supports float32 and float64 at this point");
  const auto scale =
      llnl::units::quick_convert(var.unit().underlying(), unit.underlying());
  if (std::isnan(scale))
    throw except::UnitError("Conversion from `" + to_string(var.unit()) +
                            "` to `" + to_string(unit) + "` is not valid.");
  auto out = var * astype(scale * units::one, var.dtype());
  out.setUnit(unit);
  return out;
}

} // namespace scipp::variable
