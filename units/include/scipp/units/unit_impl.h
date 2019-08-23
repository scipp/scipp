// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#ifndef SCIPP_UNITS_UNIT_IMPL_H
#define SCIPP_UNITS_UNIT_IMPL_H

#include <tuple>
#include <variant>

#include <boost/units/systems/si.hpp>
#include <boost/units/unit.hpp>

namespace scipp::units {
// Helper variables to make the declaration units more succinct.
static constexpr boost::units::si::dimensionless dimensionless;
static constexpr boost::units::si::length m;
static constexpr boost::units::si::time s;
static constexpr boost::units::si::mass kg;
static constexpr boost::units::si::temperature K;

// Define a std::variant which will hold the set of allowed units. Any unit that
// does not exist in the variant will either fail to compile or throw a
// std::runtime_error during operations such as multiplication or division.
namespace detail {
template <class... Ts, class... Extra>
std::variant<Ts...,
             decltype(std::declval<std::remove_cv_t<Ts>>() *
                      std::declval<std::remove_cv_t<Ts>>())...,
             std::remove_cv_t<Extra>...>
make_unit(const std::tuple<Ts...> &, const std::tuple<Extra...> &) {
  return {};
}

template <class... Ts> constexpr auto make_lut(std::variant<Ts...>) {
  return std::array{std::variant<Ts...>(Ts{})...};
}

} // namespace detail

template <class T, class Counts> class Unit_impl {
public:
  using unit_t = T;

  constexpr Unit_impl() = default;
  // TODO should this be explicit?
  template <class Dim, class System, class Enable>
  Unit_impl(boost::units::unit<Dim, System, Enable> unit) : m_unit(unit) {}
  explicit Unit_impl(const unit_t &unit) : m_unit(unit) {}
  static constexpr Unit_impl fromIndex(const int64_t index) {
    return Unit_impl(m_lut[index]);
  }

  constexpr const Unit_impl::unit_t &operator()() const noexcept {
    return m_unit;
  }

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
  unit_t m_unit{units::dimensionless};
  static constexpr auto m_lut{detail::make_lut(T{})};
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
