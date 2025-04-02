// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#pragma once

#include <functional>
#include <optional>
#include <string>

#include <units/unit_definitions.hpp>

#include "scipp-units_export.h"

namespace scipp::units {

class SCIPP_UNITS_EXPORT Unit {
public:
  constexpr Unit() = default;
  constexpr explicit Unit(const llnl::units::precise_unit &u) noexcept
      : m_unit(u) {}
  explicit Unit(const std::string &unit);

  [[nodiscard]] constexpr bool has_value() const noexcept {
    return m_unit.has_value();
  }
  [[nodiscard]] constexpr auto underlying() const { return m_unit.value(); }

  [[nodiscard]] std::string name() const;

  [[nodiscard]] bool isCounts() const;
  [[nodiscard]] bool isCountDensity() const;

  [[nodiscard]] bool has_same_base(const Unit &other) const;

  bool operator==(const Unit &other) const;
  bool operator!=(const Unit &other) const;

  Unit &operator+=(const Unit &other);
  Unit &operator-=(const Unit &other);
  Unit &operator*=(const Unit &other);
  Unit &operator/=(const Unit &other);
  Unit &operator%=(const Unit &other);

  template <class F> void map_over_bases(F &&f) const {
    const auto base_units = underlying().base_units();
    f("m", base_units.meter());
    f("kg", base_units.kg());
    f("s", base_units.second());
    f("A", base_units.ampere());
    f("K", base_units.kelvin());
    f("mol", base_units.mole());
    f("cd", base_units.candela());
    f("$", base_units.currency());
    f("counts", base_units.count());
    f("rad", base_units.radian());
  }

  template <class F> void map_over_flags(F &&f) const {
    const auto base_units = underlying().base_units();
    f("per_unit", base_units.is_per_unit());
    f("i_flag", base_units.has_i_flag());
    f("e_flag", base_units.has_e_flag());
    f("equation", base_units.is_equation());
  }

private:
  std::optional<llnl::units::precise_unit> m_unit;
};

SCIPP_UNITS_EXPORT Unit operator+(const Unit &a, const Unit &b);
SCIPP_UNITS_EXPORT Unit operator-(const Unit &a, const Unit &b);
SCIPP_UNITS_EXPORT Unit operator*(const Unit &a, const Unit &b);
SCIPP_UNITS_EXPORT Unit operator/(const Unit &a, const Unit &b);
SCIPP_UNITS_EXPORT Unit operator%(const Unit &a, const Unit &b);
SCIPP_UNITS_EXPORT Unit operator-(const Unit &a);
SCIPP_UNITS_EXPORT Unit abs(const Unit &a);
SCIPP_UNITS_EXPORT Unit sqrt(const Unit &a);
SCIPP_UNITS_EXPORT Unit pow(const Unit &a, int64_t power);
SCIPP_UNITS_EXPORT Unit sin(const Unit &a);
SCIPP_UNITS_EXPORT Unit cos(const Unit &a);
SCIPP_UNITS_EXPORT Unit tan(const Unit &a);
SCIPP_UNITS_EXPORT Unit asin(const Unit &a);
SCIPP_UNITS_EXPORT Unit acos(const Unit &a);
SCIPP_UNITS_EXPORT Unit atan(const Unit &a);
SCIPP_UNITS_EXPORT Unit atan2(const Unit &y, const Unit &x);
SCIPP_UNITS_EXPORT Unit sinh(const Unit &a);
SCIPP_UNITS_EXPORT Unit cosh(const Unit &a);
SCIPP_UNITS_EXPORT Unit tanh(const Unit &a);
SCIPP_UNITS_EXPORT Unit asinh(const Unit &a);
SCIPP_UNITS_EXPORT Unit acosh(const Unit &a);
SCIPP_UNITS_EXPORT Unit atanh(const Unit &a);
SCIPP_UNITS_EXPORT Unit floor(const Unit &a);
SCIPP_UNITS_EXPORT Unit ceil(const Unit &a);
SCIPP_UNITS_EXPORT Unit rint(const Unit &a);

SCIPP_UNITS_EXPORT bool identical(const Unit &a, const Unit &b);

SCIPP_UNITS_EXPORT void add_unit_alias(const std::string &name,
                                       const Unit &unit);
SCIPP_UNITS_EXPORT void clear_unit_aliases();

constexpr Unit none{};
constexpr Unit dimensionless{llnl::units::precise::one};
constexpr Unit one{llnl::units::precise::one}; /// alias for dimensionless
constexpr Unit m{llnl::units::precise::meter};
constexpr Unit s{llnl::units::precise::second};
constexpr Unit kg{llnl::units::precise::kg};
constexpr Unit K{llnl::units::precise::K};
constexpr Unit rad{llnl::units::precise::rad};
constexpr Unit deg{llnl::units::precise::deg};
constexpr Unit us{llnl::units::precise::micro * llnl::units::precise::second};
constexpr Unit ns{llnl::units::precise::ns};
constexpr Unit mm{llnl::units::precise::mm};
constexpr Unit counts{llnl::units::precise::count};
constexpr Unit angstrom{llnl::units::precise::distance::angstrom};
constexpr Unit meV{llnl::units::precise::milli *
                   llnl::units::precise::energy::eV};
constexpr Unit c{llnl::units::precise_unit{
    299792458, llnl::units::precise::m / llnl::units::precise::s}};

} // namespace scipp::units

namespace std {
template <> struct hash<scipp::units::Unit> {
  std::size_t operator()(const scipp::units::Unit &u) const {
    return hash<llnl::units::precise_unit>()(u.underlying());
  }
};
} // namespace std
