// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/core/element/to_unit.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

Variable to_unit(const VariableConstView &var, const units::Unit &unit) {
  const auto scale =
      llnl::units::quick_convert(var.unit().underlying(), unit.underlying());
  if (std::isnan(scale))
    throw except::UnitError("Conversion from `" + to_string(var.unit()) +
                            "` to `" + to_string(unit) + "` is not valid.");
  return transform(var, scale * unit, core::element::to_unit);
}

} // namespace scipp::variable
