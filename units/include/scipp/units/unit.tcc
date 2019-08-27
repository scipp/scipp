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

// Helper to check whether type is a member of a given std::variant
template <typename T, typename VARIANT_T> struct isVariantMember;
template <typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};
// Helper to make checking for allowed units more compact
template <class Units, class T> constexpr bool isKnownUnit(const T &) {
  return isVariantMember<T, Units>::value;
}

namespace units {
template <class T> std::string to_string(const T &unit) {
  return boost::lexical_cast<std::string>(unit);
}
} // namespace units

template <class T, class Counts>
std::string Unit_impl<T, Counts>::name() const {
  return std::visit([](auto &&unit) { return units::to_string(unit); }, m_unit);
}

template <class T, class Counts> bool Unit_impl<T, Counts>::isCounts() const {
  return *this == Counts();
}

template <class T, class Counts>
bool Unit_impl<T, Counts>::isCountDensity() const {
  if constexpr (std::is_same_v<Counts, decltype(dimensionless)>) {
    // Can we do anything better here? Checking for `dimensionless` in the
    // nominator could be one option, but actually it would not be correct in
    // general, e.g., for counts / velocity = s/m.
    return !isCounts();
  } else {
    return !isCounts() &&
           std::visit(
               [](auto &&unit) { return getExponent(unit, Counts()) == 1; },
               m_unit);
  }
}

template <class T, class Counts>
bool Unit_impl<T, Counts>::operator==(const Unit_impl<T, Counts> &other) const {
  return operator()() == other();
}
template <class T, class Counts>
bool Unit_impl<T, Counts>::operator!=(const Unit_impl<T, Counts> &other) const {
  return !(*this == other);
}

template <class T, class Counts>
Unit_impl<T, Counts> Unit_impl<T, Counts>::operator+=(const Unit_impl &other) {
  return *this = *this + other;
}

template <class T, class Counts>
Unit_impl<T, Counts> Unit_impl<T, Counts>::operator-=(const Unit_impl &other) {
  return *this = *this - other;
}

template <class T, class Counts>
Unit_impl<T, Counts> Unit_impl<T, Counts>::operator*=(const Unit_impl &other) {
  return *this = *this * other;
}

template <class T, class Counts>
Unit_impl<T, Counts> Unit_impl<T, Counts>::operator/=(const Unit_impl &other) {
  return *this = *this / other;
}

template <class T, class Counts>
Unit_impl<T, Counts> operator+(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  if (a == b)
    return a;
  throw except::UnitError("Cannot add " + a.name() + " and " + b.name() + ".");
}

template <class T, class Counts>
Unit_impl<T, Counts> operator-(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  if (a == b)
    return a;
  throw except::UnitError("Cannot subtract " + a.name() + " and " + b.name() +
                          ".");
}

template <class T, class... Ts>
constexpr auto make_times_inner(std::variant<Ts...>) {
  using V = std::variant<Ts...>;
  constexpr auto times_ = [](auto x, auto y) -> int64_t {
    using resultT = typename decltype(x * y)::unit_type;
    if constexpr (isKnownUnit<V>(resultT{}))
      return V(resultT{}).index();
    return -1;
  };
  return std::array{times_(T{}, Ts{})...};
}

template <class... Ts> constexpr auto make_times_lut(std::variant<Ts...>) {
  return std::array{make_times_inner<Ts>(std::variant<Ts...>{})...};
}

template <class T, class... Ts>
constexpr auto make_divide_inner(std::variant<Ts...>) {
  using V = std::variant<Ts...>;
  constexpr auto divide_ = [](auto x, auto y) -> int64_t {
    // It is done here to have the si::dimensionless then the units are
    // the same, but is the si::dimensionless valid for non si types? TODO
    if constexpr (std::is_same_v<decltype(x), decltype(y)>)
      return V(dimensionless).index();
    using resultT = typename decltype(x / y)::unit_type;
    if constexpr (isKnownUnit<V>(resultT{}))
      return V(resultT{}).index();
    return -1;
  };
  return std::array{divide_(T{}, Ts{})...};
}

template <class... Ts> constexpr auto make_divide_lut(std::variant<Ts...>) {
  return std::array{make_divide_inner<Ts>(std::variant<Ts...>{})...};
}

template <class... Ts> constexpr auto make_sqrt_lut(std::variant<Ts...>) {
  using T = std::variant<Ts...>;
  constexpr auto sqrt_ = [](auto x) -> int64_t {
    using resultT = typename decltype(sqrt(1.0 * x))::unit_type;
    if constexpr (isKnownUnit<T>(resultT{}))
      return T(resultT{}).index();
    return -1;
  };
  return std::array{sqrt_(Ts{})...};
}

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
template <class T, class Counts>
Unit_impl<T, Counts> operator*(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  static constexpr auto lut = make_times_lut(T{});
  auto resultIndex = lut[a().index()][b().index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of multiplication: (" +
                            a.name() + ") * (" + b.name() + ')');
  return Unit_impl<T, Counts>::fromIndex(resultIndex);
}

template <class T, class Counts>
Unit_impl<T, Counts> operator/(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  static constexpr auto lut = make_divide_lut(T{});
  auto resultIndex = lut[a().index()][b().index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of division: (" +
                            a.name() + ") / (" + b.name() + ')');
  return Unit_impl<T, Counts>::fromIndex(resultIndex);
}

template <class T, class Counts>
Unit_impl<T, Counts> operator-(const Unit_impl<T, Counts> &a) {
  return a;
}

template <class T, class Counts>
Unit_impl<T, Counts> abs(const Unit_impl<T, Counts> &a) {
  return a;
}

template <class T, class Counts>
Unit_impl<T, Counts> sqrt(const Unit_impl<T, Counts> &a) {
  static constexpr auto lut = make_sqrt_lut(T{});
  auto resultIndex = lut[a().index()];
  if (resultIndex < 0)
    throw except::UnitError("Unsupported unit as result of sqrt: sqrt(" +
                            a.name() + ").");
  return Unit_impl<T, Counts>::fromIndex(resultIndex);
}

#define INSTANTIATE(Units, Counts)                                             \
  template class SCIPP_UNITS_EXPORT Unit_impl<Units, Counts>;                  \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> operator+(              \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> operator-(              \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> operator*(              \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> operator/(              \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> operator-(              \
      const Unit_impl<Units, Counts> &);                                       \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> abs(                    \
      const Unit_impl<Units, Counts> &a);                                      \
  template SCIPP_UNITS_EXPORT Unit_impl<Units, Counts> sqrt(                   \
      const Unit_impl<Units, Counts> &a);

} // namespace scipp::units
