// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#pragma once

#include <array>
#include <tuple>

#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/unit.hpp>

#include "scipp-units_export.h"
#include "scipp/common/index.h"
#include "scipp/units/neutron.h"

namespace scipp::units {
// Define a std::tuple which will hold the set of allowed units. Any unit that
// does not exist in the variant will either fail to compile or throw during
// operations such as multiplication or division.
namespace detail {
template <class... Ts, class... Extra>
std::tuple<Ts...,
           decltype(std::declval<std::remove_cv_t<Ts>>() *
                    std::declval<std::remove_cv_t<Ts>>())...,
           std::remove_cv_t<Extra>...>
make_unit(const std::tuple<Ts...> &, const std::tuple<Extra...> &) {
  return {};
}

template <class Unit, class... Units>
constexpr scipp::index unit_index(Unit unit, std::tuple<Units...>) {
  // We cannot rely on matching *types* in boost units so a simple lookup of
  // `Unit` in the tuple of `Units` would often not succeed. Instead, we need to
  // use the comparison operator.
  constexpr auto match =
      std::array<bool, sizeof...(Units)>{(unit == Units{})...};
  for (size_t i = 0; i < sizeof...(Units); ++i)
    if (match[i])
      return i;
  return -1;
}

} // namespace detail

namespace boost_units {
static constexpr boost::units::si::dimensionless dimensionless;
static constexpr boost::units::si::length m;
static constexpr boost::units::si::time s;
static constexpr boost::units::si::mass kg;
static constexpr boost::units::si::temperature K;
static constexpr boost::units::si::plane_angle rad;
static constexpr decltype(boost::units::degree::plane_angle{} *
                          dimensionless) deg;

struct supported_units {
  using type = decltype(detail::make_unit(
      std::make_tuple(m, dimensionless / m, us, dimensionless / us, angstrom,
                      dimensionless / angstrom),
      std::make_tuple(dimensionless, rad, deg, rad / deg, deg / rad, counts,
                      dimensionless / counts, s, kg, meV, dimensionless / s,
                      counts / us, counts / angstrom, counts / meV, m *m *m,
                      meV *us *us / (m * m), meV *us *us *dimensionless,
                      kg *m / s, m / s, c, c *m, meV / c, dimensionless / c, K,
                      us / angstrom, us / (angstrom * angstrom),
                      us / (m * angstrom), angstrom / us, (m * angstrom) / us,
                      dimensionless / meV)));
};
struct counts_unit {
  using type = decltype(boost_units::counts);
};
} // namespace boost_units
using supported_units_t = typename boost_units::supported_units::type;
using counts_unit_t = typename boost_units::counts_unit::type;

class SCIPP_UNITS_EXPORT Unit {
public:
  constexpr Unit() = default;
  template <class Dim, class System, class Enable>
  explicit constexpr Unit(boost::units::unit<Dim, System, Enable> unit) {
    constexpr auto index = detail::unit_index(unit, supported_units_t{});
    static_assert(index >= 0, "Unsupported unit.");
    m_index = index;
  }
  static constexpr Unit fromIndex(const int64_t index) {
    Unit u;
    u.m_index = index;
    return u;
  }

  constexpr scipp::index index() const noexcept { return m_index; }

  std::string name() const;

  bool isCounts() const;
  bool isCountDensity() const;

  bool operator==(const Unit &other) const;
  bool operator!=(const Unit &other) const;

  Unit &operator+=(const Unit &other);
  Unit &operator-=(const Unit &other);
  Unit &operator*=(const Unit &other);
  Unit &operator/=(const Unit &other);

private:
  scipp::index m_index{detail::unit_index(boost::units::si::dimensionless{},
                                          supported_units_t{})};
};

Unit operator+(const Unit &a, const Unit &b);
Unit operator-(const Unit &a, const Unit &b);
Unit operator*(const Unit &a, const Unit &b);
Unit operator/(const Unit &a, const Unit &b);
Unit operator-(const Unit &a);
Unit abs(const Unit &a);
Unit sqrt(const Unit &a);
Unit sin(const Unit &a);
Unit cos(const Unit &a);
Unit tan(const Unit &a);
Unit asin(const Unit &a);
Unit acos(const Unit &a);
Unit atan(const Unit &a);
Unit atan2(const Unit &y, const Unit &x);

constexpr Unit dimensionless{boost_units::dimensionless};
constexpr Unit one{boost_units::dimensionless}; /// alias for dimensionless
constexpr Unit m{boost_units::m};
constexpr Unit s{boost_units::s};
constexpr Unit kg{boost_units::kg};
constexpr Unit K{boost_units::K};
constexpr Unit rad{boost_units::rad};
constexpr Unit deg{boost_units::deg};
constexpr Unit counts{boost_units::counts};
constexpr Unit angstrom{boost_units::angstrom};
constexpr Unit meV{boost_units::meV};
constexpr Unit us{boost_units::us};
constexpr Unit c{boost_units::c};

} // namespace scipp::units
