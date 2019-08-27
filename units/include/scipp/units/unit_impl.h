// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#ifndef SCIPP_UNITS_UNIT_IMPL_H
#define SCIPP_UNITS_UNIT_IMPL_H

#include <tuple>

#include <boost/units/systems/si.hpp>
#include <boost/units/unit.hpp>

#include "scipp/common/index.h"
#include "scipp/common/traits.h"

namespace scipp::units {
// Helper variables to make the declaration units more succinct.
static constexpr boost::units::si::dimensionless dimensionless;
static constexpr boost::units::si::length m;
static constexpr boost::units::si::time s;
static constexpr boost::units::si::mass kg;
static constexpr boost::units::si::temperature K;

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
    m_index = common::index_in_tuple<decltype(unit),
                                     supported_units_t<Derived>>::value;
  }
  static constexpr Unit_impl fromIndex(const int64_t index) {
    Unit_impl u;
    u.m_index = index;
    return u;
  }

  constexpr scipp::index index() const noexcept { return m_index; }

  std::string name() const;

  bool isCounts() const;
  bool isCountDensity() const;

  bool operator==(const Unit_impl &other) const;
  bool operator!=(const Unit_impl &other) const;

  Unit_impl &operator+=(const Unit_impl &other);
  Unit_impl &operator-=(const Unit_impl &other);
  Unit_impl &operator*=(const Unit_impl &other);
  Unit_impl &operator/=(const Unit_impl &other);

private:
  scipp::index m_index{
      common::index_in_tuple<std::decay_t<decltype(dimensionless)>,
                             supported_units_t<Derived>>::value};
  // TODO need to support scale
};

template <class Derived>
Unit_impl<Derived> operator+(const Unit_impl<Derived> &a,
                             const Unit_impl<Derived> &b);
template <class Derived>
Unit_impl<Derived> operator-(const Unit_impl<Derived> &a,
                             const Unit_impl<Derived> &b);
template <class Derived>
Unit_impl<Derived> operator*(const Unit_impl<Derived> &a,
                             const Unit_impl<Derived> &b);
template <class Derived>
Unit_impl<Derived> operator/(const Unit_impl<Derived> &a,
                             const Unit_impl<Derived> &b);
template <class Derived>
Unit_impl<Derived> operator-(const Unit_impl<Derived> &a);
template <class Derived> Unit_impl<Derived> abs(const Unit_impl<Derived> &a);
template <class Derived> Unit_impl<Derived> sqrt(const Unit_impl<Derived> &a);

} // namespace scipp::units

#endif // SCIPP_UNITS_UNIT_IMPL_H
