/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <sstream>
#include <stdexcept>

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

std::string Unit::name() const {
  return std::visit(
      [](auto &&unit) {
        std::stringstream name;
        name << unit;
        return name.str();
      },
      m_unit);
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
          return {z};
        std::stringstream msg;
        msg << "Unsupported unit as result of multiplication " << x << "*" << y;
        throw std::runtime_error(msg.str());
      },
      a.getUnit(), b.getUnit()));
}

// Dividing two units together using std::visit to run through the contents
// of the std::variant
Unit operator/(const Unit &a, const Unit &b) {
  return Unit(std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        auto z{x / y};
        if constexpr (isKnownUnit(z))
          return {z};
        std::stringstream msg;
        msg << "Unsupported unit as result of division " << x << "/" << y;
        throw std::runtime_error(msg.str());
      },
      a.getUnit(), b.getUnit()));
}

Unit operator+(const Unit &a, const Unit &b) {
  if (a != b)
    throw std::runtime_error("Cannot add different units");
  return a;
}

Unit sqrt(const Unit &a) {
  return Unit(std::visit(
      [](auto x) -> Unit::unit_t {
        typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
        if constexpr (isKnownUnit(sqrt_x))
          return {sqrt_x};
        std::stringstream msg;
        msg << "Unsupported unit as result of sqrt, sqrt(" << x << ").";
        throw std::runtime_error(msg.str());
      },
      a.getUnit()));
}
