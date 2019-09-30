// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#ifndef SCIPP_UNITS_UNIT_IMPL_H
#define SCIPP_UNITS_UNIT_IMPL_H

#include <array>
#include <tuple>

#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/unit.hpp>

#include "scipp/common/index.h"

namespace scipp::units {
// Helper variables to make the declaration units more succinct.
using dimensionless_t = boost::units::si::dimensionless;
static constexpr dimensionless_t dimensionless;
static constexpr boost::units::si::length m;
static constexpr boost::units::si::time s;
static constexpr boost::units::si::mass kg;
static constexpr boost::units::si::temperature K;
static constexpr boost::units::si::plane_angle rad;
static constexpr decltype(boost::units::degree::plane_angle{} *
                          dimensionless) deg;

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

template <class Unit> struct supported_units;
template <class Unit> struct counts_unit;
template <class Unit>
using supported_units_t = typename supported_units<Unit>::type;
template <class Unit> using counts_unit_t = typename counts_unit<Unit>::type;

template <class Derived> class SCIPP_UNITS_EXPORT Unit_impl {
public:
  constexpr Unit_impl() = default;
  // TODO should this be explicit?
  template <class Dim, class System, class Enable>
  Unit_impl(boost::units::unit<Dim, System, Enable> unit) {
    constexpr auto index =
        detail::unit_index(unit, supported_units_t<Derived>{});
    static_assert(index >= 0, "Unsupported unit.");
    m_index = index;
  }
  static constexpr Derived fromIndex(const int64_t index) {
    Derived u;
    u.m_index = index;
    return u;
  }

  constexpr scipp::index index() const noexcept { return m_index; }

  std::string name() const;

  bool isCounts() const;
  bool isCountDensity() const;

  bool operator==(const Unit_impl<Derived> &other) const;
  bool operator!=(const Unit_impl<Derived> &other) const;

  Derived &operator+=(const Unit_impl<Derived> &other);
  Derived &operator-=(const Unit_impl<Derived> &other);
  Derived &operator*=(const Unit_impl<Derived> &other);
  Derived &operator/=(const Unit_impl<Derived> &other);

private:
  scipp::index m_index{
      detail::unit_index(dimensionless, supported_units_t<Derived>{})};
  // TODO need to support scale
};

template <class Derived>
Derived operator+(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b);
template <class Derived>
Derived operator-(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b);
template <class Derived>
Derived operator*(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b);
template <class Derived>
Derived operator/(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b);
template <class Derived> Derived operator-(const Unit_impl<Derived> &a);
template <class Derived> Derived abs(const Unit_impl<Derived> &a);
template <class Derived> Derived sqrt(const Unit_impl<Derived> &a);
template <class Derived> Derived sin(const Unit_impl<Derived> &a);
template <class Derived> Derived cos(const Unit_impl<Derived> &a);
template <class Derived> Derived tan(const Unit_impl<Derived> &a);
template <class Derived> Derived asin(const Unit_impl<Derived> &a);
template <class Derived> Derived acos(const Unit_impl<Derived> &a);
template <class Derived> Derived atan(const Unit_impl<Derived> &a);

} // namespace scipp::units

#endif // SCIPP_UNITS_UNIT_IMPL_H
