// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>

#include "scipp/units/boost_units_util.h"
#include "scipp/units/except.h"
#include "scipp/units/unit_impl.h"

namespace scipp::units {

// Helper to check whether type is a member of a given std::tuple
template <typename T, typename VARIANT_T> struct isTupleMember;
template <typename T, typename... ALL_T>
struct isTupleMember<T, std::tuple<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};
// Helper to make checking for allowed units more compact
template <class Units, class T> constexpr bool isKnownUnit(const T &) {
  return isTupleMember<T, Units>::value;
}

template <class... Ts> auto make_name_lut(std::tuple<Ts...>) {
  return std::array{std::string(boost::lexical_cast<std::string>(Ts{}))...};
}

template <class Derived> std::string Unit_impl<Derived>::name() const {
  static const auto names = make_name_lut(supported_units_t<Derived>{});
  return names[index()];
}

template <class Derived> bool Unit_impl<Derived>::isCounts() const {
  return *this == counts_unit_t<Derived>();
}

template <class Counts, class... Ts>
constexpr auto make_count_density_lut(std::tuple<Ts...>) {
  if constexpr (std::is_same_v<Counts, decltype(dimensionless)>) {
    // Can we do anything better here? Checking for `dimensionless` in the
    // nominator could be one option, but actually it would not be correct in
    // general, e.g., for counts / velocity = s/m.
    return std::array{!std::is_same_v<Ts, std::decay_t<Counts>>...};
  } else {
    return std::array{(!std::is_same_v<Ts, std::decay_t<Counts>> &&
                       getExponent(Ts{}, Counts()) == 1)...};
  }
}

template <class Derived> bool Unit_impl<Derived>::isCountDensity() const {
  static constexpr auto lut = make_count_density_lut<counts_unit_t<Derived>>(
      supported_units_t<Derived>{});
  return lut[index()];
}

template <class Derived>
bool Unit_impl<Derived>::operator==(const Unit_impl<Derived> &other) const {
  return index() == other.index();
}
template <class Derived>
bool Unit_impl<Derived>::operator!=(const Unit_impl<Derived> &other) const {
  return !(*this == other);
}

template <class Derived>
Derived &Unit_impl<Derived>::operator+=(const Unit_impl &other) {
  return static_cast<Derived &>(*this = *this + other);
}

template <class Derived>
Derived &Unit_impl<Derived>::operator-=(const Unit_impl &other) {
  return static_cast<Derived &>(*this = *this - other);
}

template <class Derived>
Derived &Unit_impl<Derived>::operator*=(const Unit_impl &other) {
  return static_cast<Derived &>(*this = *this * other);
}

template <class Derived>
Derived &Unit_impl<Derived>::operator/=(const Unit_impl &other) {
  return static_cast<Derived &>(*this = *this / other);
}

template <class Derived>
Derived operator+(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b) {
  if (a == b)
    return static_cast<const Derived &>(a);
  throw except::UnitError("Cannot add " + a.name() + " and " + b.name() + ".");
}

template <class Derived>
Derived operator-(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b) {
  if (a == b)
    return static_cast<const Derived &>(a);
  throw except::UnitError("Cannot subtract " + a.name() + " and " + b.name() +
                          ".");
}

template <class T, class... Ts>
constexpr auto make_times_inner(std::tuple<Ts...>) {
  using Tuple = std::tuple<Ts...>;
  constexpr auto times_ = [](auto x, auto y) -> int64_t {
    using resultT = typename decltype(x * y)::unit_type;
    if constexpr (isKnownUnit<Tuple>(resultT{}))
      return common::index_in_tuple<resultT, Tuple>::value;
    return -1;
  };
  return std::array{times_(T{}, Ts{})...};
}

template <class... Ts> constexpr auto make_times_lut(std::tuple<Ts...>) {
  return std::array{make_times_inner<Ts>(std::tuple<Ts...>{})...};
}

template <class T, class... Ts>
constexpr auto make_divide_inner(std::tuple<Ts...>) {
  using Tuple = std::tuple<Ts...>;
  constexpr auto divide_ = [](auto x, auto y) -> int64_t {
    // It is done here to have the si::dimensionless then the units are
    // the same, but is the si::dimensionless valid for non si types? TODO
    if constexpr (std::is_same_v<decltype(x), decltype(y)>)
      return common::index_in_tuple<std::decay_t<decltype(dimensionless)>,
                                    Tuple>::value;
    using resultT = typename decltype(x / y)::unit_type;
    if constexpr (isKnownUnit<Tuple>(resultT{}))
      return common::index_in_tuple<resultT, Tuple>::value;
    return -1;
  };
  return std::array{divide_(T{}, Ts{})...};
}

template <class... Ts> constexpr auto make_divide_lut(std::tuple<Ts...>) {
  return std::array{make_divide_inner<Ts>(std::tuple<Ts...>{})...};
}

template <class... Ts> constexpr auto make_sqrt_lut(std::tuple<Ts...>) {
  using T = std::tuple<Ts...>;
  constexpr auto sqrt_ = [](auto x) -> int64_t {
    using resultT = typename decltype(sqrt(1.0 * x))::unit_type;
    if constexpr (isKnownUnit<T>(resultT{}))
      return common::index_in_tuple<resultT, T>::value;
    return -1;
  };
  return std::array{sqrt_(Ts{})...};
}

template <class Derived>
Derived operator*(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b) {
  static constexpr auto lut = make_times_lut(supported_units_t<Derived>{});
  auto resultIndex = lut[a.index()][b.index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of multiplication: (" +
                            a.name() + ") * (" + b.name() + ')');
  return Unit_impl<Derived>::fromIndex(resultIndex);
}

template <class Derived>
Derived operator/(const Unit_impl<Derived> &a, const Unit_impl<Derived> &b) {
  static constexpr auto lut = make_divide_lut(supported_units_t<Derived>{});
  auto resultIndex = lut[a.index()][b.index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of division: (" +
                            a.name() + ") / (" + b.name() + ')');
  return Unit_impl<Derived>::fromIndex(resultIndex);
}

template <class Derived> Derived operator-(const Unit_impl<Derived> &a) {
  return static_cast<const Derived &>(a);
}

template <class Derived> Derived abs(const Unit_impl<Derived> &a) {
  return static_cast<const Derived &>(a);
}

template <class Derived> Derived sqrt(const Unit_impl<Derived> &a) {
  static constexpr auto lut = make_sqrt_lut(supported_units_t<Derived>{});
  auto resultIndex = lut[a.index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of sqrt: sqrt(" +
                            a.name() + ").");
  return Unit_impl<Derived>::fromIndex(resultIndex);
}

#define INSTANTIATE(Derived)                                                   \
  template SCIPP_UNITS_EXPORT std::string Unit_impl<Derived>::name() const;    \
  template SCIPP_UNITS_EXPORT bool Unit_impl<Derived>::isCounts() const;       \
  template SCIPP_UNITS_EXPORT bool Unit_impl<Derived>::isCountDensity() const; \
  template SCIPP_UNITS_EXPORT bool Unit_impl<Derived>::operator==(             \
      const Unit_impl<Derived> &) const;                                       \
  template SCIPP_UNITS_EXPORT bool Unit_impl<Derived>::operator!=(             \
      const Unit_impl<Derived> &) const;                                       \
  template SCIPP_UNITS_EXPORT Derived &Unit_impl<Derived>::operator+=(         \
      const Unit_impl<Derived> &);                                             \
  template SCIPP_UNITS_EXPORT Derived &Unit_impl<Derived>::operator-=(         \
      const Unit_impl<Derived> &);                                             \
  template SCIPP_UNITS_EXPORT Derived &Unit_impl<Derived>::operator*=(         \
      const Unit_impl<Derived> &);                                             \
  template SCIPP_UNITS_EXPORT Derived &Unit_impl<Derived>::operator/=(         \
      const Unit_impl<Derived> &);                                             \
  template SCIPP_UNITS_EXPORT Derived operator+(const Unit_impl<Derived> &,    \
                                                const Unit_impl<Derived> &);   \
  template SCIPP_UNITS_EXPORT Derived operator-(const Unit_impl<Derived> &,    \
                                                const Unit_impl<Derived> &);   \
  template SCIPP_UNITS_EXPORT Derived operator*(const Unit_impl<Derived> &,    \
                                                const Unit_impl<Derived> &);   \
  template SCIPP_UNITS_EXPORT Derived operator/(const Unit_impl<Derived> &,    \
                                                const Unit_impl<Derived> &);   \
  template SCIPP_UNITS_EXPORT Derived operator-(const Unit_impl<Derived> &);   \
  template SCIPP_UNITS_EXPORT Derived abs(const Unit_impl<Derived> &a);        \
  template SCIPP_UNITS_EXPORT Derived sqrt(const Unit_impl<Derived> &a);

} // namespace scipp::units
