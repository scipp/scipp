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

struct supported_units {
  static constexpr boost::units::si::dimensionless dimensionless{};
  static constexpr boost::units::si::length m{};
  static constexpr boost::units::si::time s{};
  static constexpr boost::units::si::mass kg{};
  static constexpr boost::units::si::temperature K{};
  static constexpr boost::units::si::plane_angle rad{};
  static constexpr decltype(boost::units::degree::plane_angle{} *
                            dimensionless) deg{};
  // Additional helper constants beyond the SI base units.
  // Note the factor `dimensionless` in units that otherwise contain only non-SI
  // factors. This is a trick to overcome some subtleties of working with
  // heterogeneous unit systems in boost::units: We are combing SI units with
  // our own, and the two are considered independent unless you convert
  // explicitly. Therefore, in operations like (counts * m) / m, boosts is not
  // cancelling the m as expected --- you get counts * dimensionless. Explicitly
  // putting a factor dimensionless (dimensionless) into all our non-SI units
  // avoids special-case handling in all operations (which would attempt to
  // remove the dimensionless factor manually).
  static constexpr decltype(detail::tof::counts{} * dimensionless) counts{};
  static constexpr decltype(detail::tof::wavelength{} *
                            dimensionless) angstrom{};
  static constexpr decltype(detail::tof::energy{} * dimensionless) meV{};
  static constexpr decltype(detail::tof::tof{} * dimensionless) us{};
  static constexpr decltype(detail::tof::velocity{} * dimensionless) c{};

  using type = decltype(detail::make_unit(
      std::make_tuple(m, dimensionless / m),
      std::make_tuple(
          dimensionless, rad, deg, rad / deg, deg / rad, counts,
          dimensionless / counts, s, kg, angstrom, meV, us, dimensionless / us,
          dimensionless / s, counts / us, counts / angstrom, counts / meV,
          m *m *m, meV *us *us / (m * m), meV *us *us *dimensionless, kg *m / s,
          m / s, c, c *m, meV / c, dimensionless / c, K, us / angstrom,
          us / (angstrom * angstrom), us / (m * angstrom), angstrom / us,
          (m * angstrom) / us, us *us, dimensionless / (us * us),
          dimensionless / meV, dimensionless / angstrom, angstrom *angstrom,
          dimensionless / (angstrom * angstrom))));
};
struct counts_unit {
  using type = decltype(supported_units::counts);
};
using supported_units_t = typename supported_units::type;
using counts_unit_t = typename counts_unit::type;

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

constexpr Unit dimensionless{boost::units::si::dimensionless{}};
constexpr Unit m{boost::units::si::length{}};
constexpr Unit s{boost::units::si::time{}};
constexpr Unit kg{boost::units::si::mass{}};
constexpr Unit K{boost::units::si::temperature{}};
constexpr Unit rad{boost::units::si::plane_angle{}};
constexpr Unit deg{boost::units::degree::plane_angle{} *
                   boost::units::si::dimensionless{}};
constexpr Unit counts{supported_units::counts};
constexpr Unit angstrom{supported_units::angstrom};
constexpr Unit meV{supported_units::meV};
constexpr Unit us{supported_units::us};
constexpr Unit c{supported_units::c};

} // namespace scipp::units
