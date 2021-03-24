// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/core/element/to_unit.h"
#include "scipp/core/time_point.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

namespace {
constexpr double days_multiplier = llnl::units::precise::day.multiplier();
}

Variable to_unit(const Variable &var, const units::Unit &unit) {
  const auto scale =
      llnl::units::quick_convert(var.unit().underlying(), unit.underlying());
  if (std::isnan(scale))
    throw except::UnitError("Conversion from `" + to_string(var.unit()) +
                            "` to `" + to_string(unit) + "` is not valid.");
  if (var.dtype() == dtype<core::time_point> &&
      (var.unit().underlying().multiplier() >= days_multiplier ||
       unit.underlying().multiplier() >= days_multiplier)) {
    throw except::UnitError(
        "Unit conversion for datetimes with a unit of days or greater"
        " is not implemented. Attempted conversion from `" +
        to_string(var.unit()) + "` to `" + to_string(unit) + "`.");
  }
  return transform(var, scale * unit, core::element::to_unit);
}

} // namespace scipp::variable
