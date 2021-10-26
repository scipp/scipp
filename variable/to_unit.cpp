// SPDX-License-Identifier: BSD-3-Clause
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
bool involves_unit_greater_than_days(const Variable &var,
                                     const units::Unit &unit) {
  constexpr auto days_multiplier = llnl::units::precise::day.multiplier();
  return variableFactory().elem_unit(var).underlying().multiplier() >=
             days_multiplier ||
         unit.underlying().multiplier() >= days_multiplier;
}
} // namespace

Variable to_unit(const Variable &var, const units::Unit &unit,
                 const CopyPolicy copy) {
  if (unit == var.unit())
    return copy == CopyPolicy::Always ? variable::copy(var) : var;
  const auto scale = llnl::units::quick_convert(
      variableFactory().elem_unit(var).underlying(), unit.underlying());
  if (std::isnan(scale))
    throw except::UnitError("Conversion from `" + to_string(var.unit()) +
                            "` to `" + to_string(unit) + "` is not valid.");
  if (var.dtype() == dtype<core::time_point> &&
      involves_unit_greater_than_days(var, unit)) {
    throw except::UnitError(
        "Unit conversions for datetimes with a unit of days or greater "
        "are not supported. Attempted conversion from `" +
        to_string(var.unit()) + "` to `" + to_string(unit) + "`. " +
        "This limitation exists because such conversions would require "
        "information about calendars and time zones.");
  }
  return variable::transform(var, scale * unit, core::element::to_unit,
                             "to_unit");
}

} // namespace scipp::variable
