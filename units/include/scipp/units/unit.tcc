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
#include "scipp/units/unit_impl.h"

namespace scipp::units {

// Helper to check whether type is a member of a given std::variant
template <typename T, typename VARIANT_T> struct isVariantMember;
template <typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};
// Helper to make checking for allowed units more compact
template <class Units, class Counts, class T>
constexpr bool isKnownUnit(const T &) {
  return isVariantMember<T, typename Unit_impl<Units, Counts>::unit_t>::value;
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
Unit_impl<T, Counts> operator+(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  if (a == b)
    return a;
  throw std::runtime_error("Cannot add " + a.name() + " and " + b.name() + ".");
}

template <class T, class Counts>
Unit_impl<T, Counts> operator-(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  if (a == b)
    return a;
  throw std::runtime_error("Cannot subtract " + a.name() + " and " + b.name() +
                           ".");
}

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
template <class T, class Counts>
Unit_impl<T, Counts> operator*(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  return Unit_impl<T, Counts>(std::visit(
      [](auto x, auto y) -> typename Unit_impl<T, Counts>::unit_t {
        // Creation of z needed here because putting x*y inside the call to
        // isKnownUnit(x*y) leads to error: temporary of non-literal type in a
        // constant expression
        auto z{x * y};
        if constexpr (isKnownUnit<T, Counts>(z))
          return z;
        throw std::runtime_error(
            "Unsupported unit as result of multiplication: (" +
            units::to_string(x) + ") * (" + units::to_string(y) + ')');
      },
      a(), b()));
}

template <class T, class Counts>
Unit_impl<T, Counts> operator/(const Unit_impl<T, Counts> &a,
                               const Unit_impl<T, Counts> &b) {
  return Unit_impl<T, Counts>(std::visit(
      [](auto x, auto y) -> typename Unit_impl<T, Counts>::unit_t {
        // It is done here to have the si::dimensionless then the units are
        // the same, but is the si::dimensionless valid for non si types? TODO
        if constexpr (std::is_same_v<decltype(x), decltype(y)>)
          return dimensionless;
        auto z{x / y};
        if constexpr (isKnownUnit<T, Counts>(z))
          return z;
        throw std::runtime_error("Unsupported unit as result of division: (" +
                                 units::to_string(x) + ") / (" +
                                 units::to_string(y) + ')');
      },
      a(), b()));
}

template <class T, class Counts>
Unit_impl<T, Counts> sqrt(const Unit_impl<T, Counts> &a) {
  return Unit_impl<T, Counts>(std::visit(
      [](auto x) -> typename Unit_impl<T, Counts>::unit_t {
        typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
        if constexpr (isKnownUnit<T, Counts>(sqrt_x))
          return sqrt_x;
        throw std::runtime_error("Unsupported unit as result of sqrt: sqrt(" +
                                 units::to_string(x) + ").");
      },
      a()));
}

#define INSTANTIATE(Units, Counts)                                             \
  template class Unit_impl<Units, Counts>;                                     \
  template Unit_impl<Units, Counts> operator+(                                 \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template Unit_impl<Units, Counts> operator-(                                 \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template Unit_impl<Units, Counts> operator*(                                 \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template Unit_impl<Units, Counts> operator/(                                 \
      const Unit_impl<Units, Counts> &, const Unit_impl<Units, Counts> &);     \
  template Unit_impl<Units, Counts> sqrt(const Unit_impl<Units, Counts> &a);

} // namespace scipp::units
