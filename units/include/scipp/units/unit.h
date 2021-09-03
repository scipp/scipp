// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#pragma once

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

  [[nodiscard]] constexpr auto underlying() const noexcept { return m_unit; }

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

private:
  llnl::units::precise_unit m_unit;
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
SCIPP_UNITS_EXPORT Unit floor(const Unit &a);
SCIPP_UNITS_EXPORT Unit ceil(const Unit &a);
SCIPP_UNITS_EXPORT Unit rint(const Unit &a);

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
constexpr Unit c{
    {llnl::units::precise::m / llnl::units::precise::s, 299792458}};

} // namespace scipp::units
