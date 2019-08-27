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

template <class T, class Counts> class Unit_impl {
public:
  constexpr Unit_impl() = default;
  // TODO should this be explicit?
  template <class Dim, class System, class Enable>
  Unit_impl(boost::units::unit<Dim, System, Enable> unit) {
    m_index = common::index_in_tuple<decltype(unit), T>::value;
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

  bool operator==(const Unit_impl<T, Counts> &other) const;
  bool operator!=(const Unit_impl<T, Counts> &other) const;

  Unit_impl operator+=(const Unit_impl &other);
  Unit_impl operator-=(const Unit_impl &other);
  Unit_impl operator*=(const Unit_impl &other);
  Unit_impl operator/=(const Unit_impl &other);

private:
  scipp::index m_index{
      common::index_in_tuple<std::decay_t<decltype(dimensionless)>, T>::value};
  // TODO need to support scale
};

template <class T, class Counts>
Unit_impl<T, Counts> operator+(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b);
template <class T, class Counts>
Unit_impl<T, Counts> operator-(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b);
template <class T, class Counts>
Unit_impl<T, Counts> operator*(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b);
template <class T, class Counts>
Unit_impl<T, Counts> operator/(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b);
template <class T, class Counts>
Unit_impl<T, Counts> operator-(const Unit_impl<T, Counts> &a);
template <class T, class Counts>
Unit_impl<T, Counts> abs(const Unit_impl<T, Counts> &a);
template <class T, class Counts>
Unit_impl<T, Counts> sqrt(const Unit_impl<T, Counts> &a);

} // namespace scipp::units

#endif // SCIPP_UNITS_UNIT_IMPL_H
