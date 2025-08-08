// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>
#include <units/units.hpp>

#include "scipp/core/element/to_unit.h"
#include "scipp/core/time_point.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/to_unit.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

namespace {
bool greater_than_days(const sc_units::Unit &unit) {
  static constexpr auto days_multiplier = units::precise::day.multiplier();
  if (!unit.has_same_base(sc_units::s)) {
    throw except::UnitError("Cannot convert unit of datetime with non-time "
                            "unit, got `" +
                            to_string(unit) + "`.");
  }
  return unit.underlying().multiplier() >= days_multiplier;
}
} // namespace

Variable to_unit(const Variable &var, const sc_units::Unit &unit,
                 const CopyPolicy copy) {
  const auto var_unit = variableFactory().elem_unit(var);
  if (unit == var_unit)
    return copy == CopyPolicy::Always ? variable::copy(var) : var;
  if ((var_unit == sc_units::none) || (unit == sc_units::none))
    throw except::UnitError("Unit conversion to / from None is not permitted.");
  const auto scale =
      units::quick_convert(var_unit.underlying(), unit.underlying());
  if (std::isnan(scale))
    throw except::UnitError("Conversion from `" + to_string(var_unit) +
                            "` to `" + to_string(unit) + "` is not valid.");
  if (var.dtype() == dtype<core::time_point> &&
      (greater_than_days(variableFactory().elem_unit(var)) ||
       greater_than_days(unit))) {
    throw except::UnitError(
        "Unit conversions for datetimes with a unit of days or greater "
        "are not supported. Attempted conversion from `" +
        to_string(var_unit) + "` to `" + to_string(unit) + "`. " +
        "This limitation exists because such conversions would require "
        "information about calendars and time zones.");
  }
  Variable scalevar;
  // Need to make sure that errors due to machine precision actually affect
  // decimal places, otherwise the approach based on std::round will do nothing.
  const auto base_scale = scale > 1e6 ? scale * 1e-6 : scale;
  if (const auto iscale = std::round(base_scale);
      (std::abs(base_scale - iscale) < 1e-12 * std::abs(base_scale))) {
    if (var.dtype() == dtype<int64_t> || var.dtype() == dtype<core::time_point>)
      scalevar = int64_t{scale > 1e6 ? 1000000 : 1} *
                 static_cast<int64_t>(iscale) * unit;
    else
      scalevar = (scale > 1e6 ? 1000000 : 1) * iscale * unit;
  } else {
    scalevar = scale * unit;
  }
  return variable::transform(var, scalevar, core::element::to_unit, "to_unit");
}

} // namespace scipp::variable
