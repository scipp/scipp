/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>

#include "unit.h"

using namespace units;

// Helper to check whether type is a member of a given std::variant
template <typename T, typename VARIANT_T> struct isVariantMember;
template <typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};
// Helper to make checking for allowed units more compact
template <class T> constexpr bool isKnownUnit(const T &) {
  return isVariantMember<T, Unit::unit_t>::value;
}

namespace units {
template <class T> std::string to_string(const T &unit) {
  return boost::lexical_cast<std::string>(unit);
}
} // namespace units

std::string Unit::name() const {
  return std::visit([](auto &&unit) { return units::to_string(unit); }, m_unit);
}

bool operator==(const Unit &a, const Unit &b) { return a() == b(); }
bool operator!=(const Unit &a, const Unit &b) { return !(a == b); }

Unit operator+(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw std::runtime_error("Cannot add " + a.name() + " and " + b.name() + ".");
}

Unit operator-(const Unit &a, const Unit &b) {
  if (a == b)
    return a;
  throw std::runtime_error("Cannot subtract " + a.name() + " and " + b.name() +
                           ".");
}

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
Unit operator*(const Unit &a, const Unit &b) {
  return Unit(std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        // Creation of z needed here because putting x*y inside the call to
        // isKnownUnit(x*y) leads to error: temporary of non-literal type in
        // a constant expression
        auto z{x * y};
        if constexpr (isKnownUnit(z))
          return z;
        throw std::runtime_error(
            "Unsupported unit as result of multiplication: (" +
            units::to_string(x) + ") * (" + units::to_string(y) + ')');
      },
      a(), b()));
}

Unit operator/(const Unit &a, const Unit &b) {
  return Unit(std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        auto z{x / y};
        if constexpr (isKnownUnit(z))
          return z;
        throw std::runtime_error("Unsupported unit as result of division: (" +
                                 units::to_string(x) + ") / (" +
                                 units::to_string(y) + ')');
      },
      a(), b()));
}

Unit sqrt(const Unit &a) {
  return Unit(std::visit(
      [](auto x) -> Unit::unit_t {
        typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
        if constexpr (isKnownUnit(sqrt_x))
          return sqrt_x;
        throw std::runtime_error("Unsupported unit as result of sqrt: sqrt(" +
                                 units::to_string(x) + ").");
      },
      a()));
}

namespace units {
bool containsCounts(const Unit &unit) {
  if ((unit == units::counts) || unit == units::counts / units::us)
    return true;
  return false;
}
bool containsCountsVariance(const Unit &unit) {
  if (unit == units::counts * units::counts ||
      unit == (units::counts / units::us) * (units::counts / units::us))
    return true;
  return false;
}
} // namespace units
