/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "unit.h"
#include <stdexcept>

// Helper type for the visitor id()
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;
// Helper to check whether type is a member of a given std::variant
template <typename T, typename VARIANT_T> struct isVariantMember;
template <typename T, typename... ALL_T>
struct isVariantMember<T, std::variant<ALL_T...>>
    : public std::disjunction<std::is_same<T, ALL_T>...> {};

/// Construct unit from a given Id
Unit::Unit(const Unit::Id id) {
  switch (id) {
  case Unit::Id::Dimensionless:
    m_unit = boost::units::si::dimensionless();
    break;
  case Unit::Id::Length:
    m_unit = boost::units::si::length();
    break;
  case Unit::Id::Area:
    m_unit = boost::units::si::area();
    break;
  case Unit::Id::AreaVariance:
    m_unit = boost::units::si::area() * boost::units::si::area();
    break;
  case Unit::Id::Counts:
    m_unit = datasetunits::counts();
    break;
  case Unit::Id::CountsVariance:
    m_unit = datasetunits::counts() * datasetunits::counts();
    break;
  case Unit::Id::InverseLength:
    m_unit = boost::units::si::dimensionless() / boost::units::si::length();
    break;
  case Unit::Id::Energy:
    m_unit = datasetunits::energy();
    break;
  case Unit::Id::Wavelength:
    m_unit = datasetunits::wavelength();
    break;
  case Unit::Id::Time:
    m_unit = boost::units::si::time();
    break;
  case Unit::Id::Tof:
    m_unit = datasetunits::tof();
    break;
  case Unit::Id::Mass:
    m_unit = boost::units::si::mass();
    break;
  }
}

/// Get the Id corresponding to the underlying unit
Unit::Id Unit::id() const {
  // return Unit::Id::Dimensionless;
  return std::visit(
      overloaded{[](boost::units::si::dimensionless) {
                   return Unit::Id::Dimensionless;
                 },
                 [](boost::units::si::length) { return Unit::Id::Length; },
                 [](boost::units::si::area) { return Unit::Id::Area; },
                 [](decltype(std::declval<boost::units::si::area>() *
                             std::declval<boost::units::si::area>())) {
                   return Unit::Id::AreaVariance;
                 },
                 [](datasetunits::counts) { return Unit::Id::Counts; },
                 [](decltype(std::declval<datasetunits::counts>() *
                             std::declval<datasetunits::counts>())) {
                   return Unit::Id::CountsVariance;
                 },
                 [](decltype(std::declval<boost::units::si::dimensionless>() /
                             std::declval<boost::units::si::length>())) {
                   return Unit::Id::InverseLength;
                 },
                 [](datasetunits::energy) { return Unit::Id::Energy; },
                 [](datasetunits::wavelength) { return Unit::Id::Wavelength; },
                 [](boost::units::si::time) { return Unit::Id::Time; },
                 [](datasetunits::tof) { return Unit::Id::Tof; },
                 [](boost::units::si::mass) { return Unit::Id::Mass; },
                 [](auto) -> Unit::Id {
                   throw std::runtime_error("Unit not yet implemented");
                 }},
      m_unit);
}

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
Unit operator*(const Unit &a, const Unit &b) {
  return std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        if constexpr (std::is_same<decltype(x),
                                   boost::units::si::dimensionless>::value)
          return {y};
        else if constexpr (std::is_same<decltype(y),
                                        boost::units::si::dimensionless>::value)
          return {x};
        else if constexpr (isVariantMember<decltype(x * y),
                                           Unit::unit_t>::value)
          return {x * y};
        else
          throw std::runtime_error(
              "Unsupported unit combination in multiplication");
      },
      a.getUnit(), b.getUnit());
}

// Dividing two units together using std::visit to run through the contents
// of the std::variant
Unit operator/(const Unit &a, const Unit &b) {
  return std::visit(
      [](auto x, auto y) -> Unit::unit_t {
        if constexpr (std::is_same<decltype(y),
                                   boost::units::si::dimensionless>::value)
          return {x};
        else if constexpr (isVariantMember<decltype(x / y),
                                           Unit::unit_t>::value)
          return {x / y};
        else
          throw std::runtime_error("Unsupported unit combination in division");
      },
      a.getUnit(), b.getUnit());
}

Unit operator+(const Unit &a, const Unit &b) {
  if (a != b)
    throw std::runtime_error("Cannot add different units");
  return a;
}
