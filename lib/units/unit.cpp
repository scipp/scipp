// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#include <regex>
#include <stdexcept>

#include <units/units.hpp>
#include <units/units_util.hpp>

#include "scipp/units/except.h"
#include "scipp/units/unit.h"

namespace scipp::sc_units {

namespace {
std::string map_unit_string(const std::string &unit) {
  // custom dimensionless name
  return unit == "dimensionless" ? ""
         // Use Gregorian months and years by default.
         : unit == "y" || unit == "Y" || unit == "year" ? "a_g"
         // Overwrite M to mean month instead of molarity for numpy
         // interop.
         : unit == "M" || unit == "month" ? "mog"
                                          : unit;
}

bool is_special_unit(const units::precise_unit &unit) {
  using namespace units::precise::custom;
  const auto &base = unit.base_units();

  // Allowing custom_count_unit_number == 1 because that is 'arbitrary unit'
  return is_custom_unit(base) ||
         (is_custom_count_unit(base) && custom_count_unit_number(base) != 1) ||
         unit.commodity() != 0;
}
} // namespace

Unit::Unit(const std::string &unit)
    : Unit(units::unit_from_string(map_unit_string(unit), units::strict_si)) {
  if (const auto &u = m_unit.value(); is_special_unit(u) || !is_valid(u))
    throw except::UnitError("Failed to convert string `" + unit +
                            "` to valid unit.");
}

std::string Unit::name() const {
  if (!has_value())
    return "None";
  if (*this == Unit{"month"}) {
    return "M";
  }
  auto repr = to_string(*m_unit);
  repr = std::regex_replace(repr, std::regex("^u"), "µ");
  repr = std::regex_replace(repr, std::regex("item"), "count");
  repr = std::regex_replace(repr, std::regex("count(?!s)"), "counts");
  repr = std::regex_replace(repr, std::regex("day"), "D");
  repr = std::regex_replace(repr, std::regex("a_g"), "Y");
  return repr.empty() ? "dimensionless" : repr;
}

bool Unit::isCounts() const { return *this == counts; }

bool Unit::isCountDensity() const {
  return has_value() && !isCounts() && m_unit->base_units().count() != 0;
}

bool Unit::has_same_base(const Unit &other) const {
  return has_value() && m_unit->has_same_base(other.underlying());
}

bool Unit::operator==(const Unit &other) const {
  return m_unit == other.m_unit;
}

bool Unit::operator!=(const Unit &other) const { return !(*this == other); }

Unit &Unit::operator+=(const Unit &other) { return *this = *this + other; }

Unit &Unit::operator-=(const Unit &other) { return *this = *this - other; }

Unit &Unit::operator*=(const Unit &other) { return *this = *this * other; }

Unit &Unit::operator/=(const Unit &other) { return *this = *this / other; }

Unit &Unit::operator%=(const Unit &other) { return *this = *this % other; }

Unit operator+(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw except::UnitError("Cannot add " + a.name() + " and " + b.name() + ".");
}

Unit operator-(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw except::UnitError("Cannot subtract " + a.name() + " and " + b.name() +
                          ".");
}

namespace {
void expect_not_none(const Unit &u, const std::string &name) {
  if (!u.has_value())
    throw except::UnitError("Cannot " + name + " with operand of unit 'None'.");
}
} // namespace

Unit operator*(const Unit &a, const Unit &b) {
  if (a == none && b == none)
    return none;
  expect_not_none(a, "multiply");
  expect_not_none(b, "multiply");
  if (units::times_overflows(a.underlying(), b.underlying()))
    throw except::UnitError("Unsupported unit as result of multiplication: (" +
                            a.name() + ") * (" + b.name() + ')');
  return Unit{a.underlying() * b.underlying()};
}

Unit operator/(const Unit &a, const Unit &b) {
  if (a == none && b == none)
    return none;
  expect_not_none(a, "divide");
  expect_not_none(b, "divide");
  if (units::divides_overflows(a.underlying(), b.underlying()))
    throw except::UnitError("Unsupported unit as result of division: (" +
                            a.name() + ") / (" + b.name() + ')');
  return Unit{a.underlying() / b.underlying()};
}

Unit operator%(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw except::UnitError("Cannot perform modulo operation with " + a.name() +
                          " and " + b.name() + ". Units must be the same.");
}

Unit operator-(const Unit &a) { return a; }

Unit abs(const Unit &a) { return a; }

Unit floor(const Unit &a) { return a; }

Unit ceil(const Unit &a) { return a; }

Unit rint(const Unit &a) { return a; }

Unit sqrt(const Unit &a) {
  if (a == none)
    return a;
  if (units::is_error(sqrt(a.underlying())))
    throw except::UnitError("Unsupported unit as result of sqrt: sqrt(" +
                            a.name() + ").");
  return Unit{sqrt(a.underlying())};
}

Unit pow(const Unit &a, const int64_t power) {
  if (a == none)
    return a;
  if (units::pow_overflows(a.underlying(), static_cast<int>(power)))
    throw except::UnitError("Unsupported unit as result of pow: pow(" +
                            a.name() + ", " + std::to_string(power) + ").");
  return Unit{a.underlying().pow(static_cast<int>(power))};
}

Unit trigonometric(const Unit &a) {
  if (a == sc_units::rad || a == sc_units::deg)
    return sc_units::dimensionless;
  throw except::UnitError(
      "Trigonometric function requires rad or deg unit, got " + a.name() + ".");
}

Unit inverse_trigonometric(const Unit &a) {
  if (a == sc_units::dimensionless)
    return sc_units::rad;
  throw except::UnitError(
      "Inverse trigonometric function requires dimensionless unit, got " +
      a.name() + ".");
}

Unit sin(const Unit &a) { return trigonometric(a); }
Unit cos(const Unit &a) { return trigonometric(a); }
Unit tan(const Unit &a) { return trigonometric(a); }
Unit asin(const Unit &a) { return inverse_trigonometric(a); }
Unit acos(const Unit &a) { return inverse_trigonometric(a); }
Unit atan(const Unit &a) { return inverse_trigonometric(a); }
Unit atan2(const Unit &y, const Unit &x) {
  expect_not_none(x, "atan2");
  expect_not_none(y, "atan2");
  if (x == y)
    return sc_units::rad;
  throw except::UnitError(
      "atan2 function requires matching units for input, got a " + x.name() +
      " b " + y.name() + ".");
}

Unit hyperbolic(const Unit &a) {
  if (a == sc_units::dimensionless)
    return sc_units::dimensionless;
  throw except::UnitError(
      "Hyperbolic function requires dimensionless input, got " + a.name() +
      ".");
}

Unit sinh(const Unit &a) { return hyperbolic(a); }
Unit cosh(const Unit &a) { return hyperbolic(a); }
Unit tanh(const Unit &a) { return hyperbolic(a); }
Unit asinh(const Unit &a) { return hyperbolic(a); }
Unit acosh(const Unit &a) { return hyperbolic(a); }
Unit atanh(const Unit &a) { return hyperbolic(a); }

bool identical(const Unit &a, const Unit &b) {
  return a.has_value() && b.has_value() &&
         a.underlying().is_exactly_the_same(b.underlying());
}

void add_unit_alias(const std::string &name, const Unit &unit) {
  units::addUserDefinedUnit(name, unit.underlying());
}

void clear_unit_aliases() { units::clearUserDefinedUnits(); }

} // namespace scipp::sc_units
