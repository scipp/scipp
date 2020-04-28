// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>

#include "scipp/units/boost_units_util.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

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

std::string Unit::name() const {
  static const auto names = make_name_lut(supported_units_t{});
  return names[index()];
}

bool Unit::isCounts() const { return *this == Unit(counts_unit_t{}); }

template <class Counts, class... Ts>
constexpr auto make_count_density_lut(std::tuple<Ts...>) {
  if constexpr (std::is_same_v<Counts, boost::units::si::dimensionless>) {
    // Can we do anything better here? Checking for `dimensionless` in the
    // nominator could be one option, but actually it would not be correct in
    // general, e.g., for counts / velocity = s/m.
    return std::array{!std::is_same_v<Ts, std::decay_t<Counts>>...};
  } else {
    return std::array{(!std::is_same_v<Ts, std::decay_t<Counts>> &&
                       getExponent(Ts{}, Counts()) == 1)...};
  }
}

bool Unit::isCountDensity() const {
  static constexpr auto lut =
      make_count_density_lut<counts_unit_t>(supported_units_t{});
  return lut[index()];
}

bool Unit::operator==(const Unit &other) const {
  return index() == other.index();
}
bool Unit::operator!=(const Unit &other) const { return !(*this == other); }

Unit &Unit::operator+=(const Unit &other) {
  return static_cast<Unit &>(*this = *this + other);
}

Unit &Unit::operator-=(const Unit &other) {
  return static_cast<Unit &>(*this = *this - other);
}

Unit &Unit::operator*=(const Unit &other) {
  return static_cast<Unit &>(*this = *this * other);
}

Unit &Unit::operator/=(const Unit &other) {
  return static_cast<Unit &>(*this = *this / other);
}

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

template <class T, class... Ts>
constexpr auto make_times_inner(std::tuple<Ts...>) {
  using Tuple = std::tuple<Ts...>;
  constexpr auto times_ = [](auto x, auto y) -> int64_t {
    using resultT = typename decltype(x * y)::unit_type;
    return detail::unit_index(resultT{}, Tuple{});
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
    using resultT = typename decltype(x / y)::unit_type;
    return detail::unit_index(resultT{}, Tuple{});
  };
  return std::array{divide_(T{}, Ts{})...};
}

template <class... Ts> constexpr auto make_divide_lut(std::tuple<Ts...>) {
  return std::array{make_divide_inner<Ts>(std::tuple<Ts...>{})...};
}

template <class... Ts> constexpr auto make_sqrt_lut(std::tuple<Ts...>) {
  using Tuple = std::tuple<Ts...>;
  constexpr auto sqrt_ = [](auto x) -> int64_t {
    using resultT = typename decltype(sqrt(1.0 * x))::unit_type;
    return detail::unit_index(resultT{}, Tuple{});
  };
  return std::array{sqrt_(Ts{})...};
}

Unit operator*(const Unit &a, const Unit &b) {
  static constexpr auto lut = make_times_lut(supported_units_t{});
  auto resultIndex = lut[a.index()][b.index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of multiplication: (" +
                            a.name() + ") * (" + b.name() + ')');
  return Unit::fromIndex(resultIndex);
}

Unit operator/(const Unit &a, const Unit &b) {
  static constexpr auto lut = make_divide_lut(supported_units_t{});
  auto resultIndex = lut[a.index()][b.index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of division: (" +
                            a.name() + ") / (" + b.name() + ')');
  return Unit::fromIndex(resultIndex);
}

Unit operator-(const Unit &a) { return static_cast<const Unit &>(a); }

Unit abs(const Unit &a) { return static_cast<const Unit &>(a); }

Unit sqrt(const Unit &a) {
  static constexpr auto lut = make_sqrt_lut(supported_units_t{});
  auto resultIndex = lut[a.index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of sqrt: sqrt(" +
                            a.name() + ").");
  return Unit::fromIndex(resultIndex);
}

Unit trigonometric(const Unit &a) {
  if (a == units::rad || a == units::deg)
    return units::dimensionless;
  throw except::UnitError(
      "Trigonometric function requires rad or deg unit, got " + a.name() + ".");
}

Unit inverse_trigonometric(const Unit &a) {
  if (a == units::dimensionless)
    return units::rad;
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
  if (x == y)
    return units::rad;
  throw except::UnitError(
      "atan2 function requires matching units for input, got a " + x.name() +
      " b " + y.name() + ".");
}

} // namespace scipp::units
