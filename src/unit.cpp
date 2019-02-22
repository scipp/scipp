/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <stdexcept>

#include <boost/units/cmath.hpp>
#include <boost/units/io.hpp>

#include "unit.h"

using namespace units;

// Helper type for the visitor id()
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;
// Helper to check whether type is a member of a given std::variant
template <typename T, typename VARIANT_T> struct isVariantMember;
template <typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};
// Helper to make checking for allowed units more compact
template <class T> constexpr bool isKnownUnit(const T &) {
  return isVariantMember<T, Unit::unit_t>::value;
}

/// Construct unit from a given Id
Unit::Unit(const Unit::Id id) {
  switch (id) {
  case Unit::Id::Dimensionless:
    m_unit = none;
    break;
  case Unit::Id::Length:
    m_unit = m;
    break;
  case Unit::Id::Area:
    m_unit = m2;
    break;
  case Unit::Id::AreaVariance:
    m_unit = m2 * m2;
    break;
  case Unit::Id::Counts:
    m_unit = counts * none;
    break;
  case Unit::Id::CountsVariance:
    m_unit = counts * counts * none;
    break;
  case Unit::Id::CountsPerMeter:
    m_unit = counts / m;
    break;
  case Unit::Id::InverseLength:
    m_unit = none / m;
    break;
  case Unit::Id::InverseTime:
    m_unit = none / s;
    break;
  case Unit::Id::Energy:
    m_unit = meV * none;
    break;
  case Unit::Id::Wavelength:
    m_unit = lambda * none;
    break;
  case Unit::Id::Time:
    m_unit = s;
    break;
  case Unit::Id::Tof:
    m_unit = tof * none;
    break;
  case Unit::Id::Mass:
    m_unit = kg;
    break;
  default:
    throw std::runtime_error("Unsupported Id in Unit constructor");
  }
}

/// Get the Id corresponding to the underlying unit
Unit::Id Unit::id() const {
  return std::visit(
      overloaded{[](decltype(none)) { return Unit::Id::Dimensionless; },
                 [](decltype(m)) { return Unit::Id::Length; },
                 [](decltype(m2)) { return Unit::Id::Area; },
                 [](decltype(m2 * m2)) { return Unit::Id::AreaVariance; },
                 [](decltype(counts * none)) { return Unit::Id::Counts; },
                 [](decltype(counts * counts * none)) {
                   return Unit::Id::CountsVariance;
                 },
                 [](decltype(counts / m)) { return Unit::Id::CountsPerMeter; },
                 [](decltype(none / m)) { return Unit::Id::InverseLength; },
                 [](decltype(none / s)) { return Unit::Id::InverseTime; },
                 [](decltype(meV * none)) { return Unit::Id::Energy; },
                 [](decltype(lambda * none)) { return Unit::Id::Wavelength; },
                 [](decltype(s)) { return Unit::Id::Time; },
                 [](decltype(tof * none)) { return Unit::Id::Tof; },
                 [](decltype(kg)) { return Unit::Id::Mass; },
                 [](auto unit) -> Unit::Id {
                   std::stringstream msg;
                   msg << "Unsupported unit " << unit;
                   throw std::runtime_error(msg.str());
                 }},
      m_unit);
}

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
Unit operator*(const Unit &a, const Unit &b) {
  return std::visit(
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
      a.getUnit(), b.getUnit());
}

// Dividing two units together using std::visit to run through the contents
// of the std::variant
Unit operator/(const Unit &a, const Unit &b) {
  return std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        auto z{x / y};
        if constexpr (isKnownUnit(z))
          return {z};
        std::stringstream msg;
        msg << "Unsupported unit as result of division " << x << "/" << y;
        throw std::runtime_error(msg.str());
      },
      a.getUnit(), b.getUnit());
}

Unit operator+(const Unit &a, const Unit &b) {
  if (a != b)
    throw std::runtime_error("Cannot add different units");
  return a;
}

Unit sqrt(const Unit &a) {
  return std::visit(
      [](auto x) -> Unit::unit_t {
        typename decltype(sqrt(1.0 * x))::unit_type sqrt_x;
        if constexpr (isKnownUnit(sqrt_x))
          return {sqrt_x};
        std::stringstream msg;
        msg << "Unsupported unit as result of sqrt, sqrt(" << x << ").";
        throw std::runtime_error(msg.str());
      },
      a.getUnit());
}
