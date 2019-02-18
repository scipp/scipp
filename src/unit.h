/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef UNIT_H
#define UNIT_H

#include <boost/units/base_dimension.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/unit.hpp>
#include <variant>

namespace datasetunits {

// Base dimension of counts
struct counts_base_dimension
    : boost::units::base_dimension<counts_base_dimension, 1> {};
typedef counts_base_dimension::dimension_type counts_dimension;

// TODO: do we need to add some derived units here?

// Base units from the base dimensions
struct counts_base_unit
    : boost::units::base_unit<counts_base_unit, counts_dimension, 1> {};
struct wavelength_base_unit
    : boost::units::base_unit<wavelength_base_unit,
                              boost::units::length_dimension, 2> {};
struct energy_base_unit
    : boost::units::base_unit<energy_base_unit, boost::units::energy_dimension,
                              3> {};
struct tof_base_unit
    : boost::units::base_unit<tof_base_unit, boost::units::time_dimension, 4> {
};

// Create the units system using the make_system utility
typedef boost::units::make_system<wavelength_base_unit, counts_base_unit,
                                  energy_base_unit, tof_base_unit>::type
    units_system;

typedef boost::units::unit<counts_dimension, units_system> counts;
typedef boost::units::unit<boost::units::length_dimension, units_system>
    wavelength;
typedef boost::units::unit<boost::units::energy_dimension, units_system> energy;
typedef boost::units::unit<boost::units::time_dimension, units_system> tof;

/// unit constants
BOOST_UNITS_STATIC_CONSTANT(lambda, wavelength);
BOOST_UNITS_STATIC_CONSTANT(lambdas, wavelength);
BOOST_UNITS_STATIC_CONSTANT(meV, energy);
BOOST_UNITS_STATIC_CONSTANT(meVs, energy);
BOOST_UNITS_STATIC_CONSTANT(microsecond, tof);
BOOST_UNITS_STATIC_CONSTANT(microseconds, tof);

} // namespace datasetunits

// Define conversion factors: the conversion will work both ways with a single
// macro

// Convert angstroms to SI meters
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(datasetunits::wavelength_base_unit,
                                     boost::units::si::length, double, 1.0e-10);
// Convert meV to SI Joule
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    datasetunits::energy_base_unit, boost::units::si::energy, double,
    1.0e-3 * boost::units::si::constants::codata::e.value().value());
// Convert tof microseconds to SI seconds
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(datasetunits::tof_base_unit,
                                     boost::units::si::time, double, 1.0e-6);

template <>
struct boost::units::base_unit_info<datasetunits::wavelength_base_unit> {
  static std::string name() { return "angstroms"; }
  static std::string symbol() { return "AA"; }
};

template <>
struct boost::units::base_unit_info<datasetunits::counts_base_unit> {
  static std::string name() { return "counts"; }
};

template <>
struct boost::units::base_unit_info<datasetunits::energy_base_unit> {
  static std::string name() { return "milli-electronvolt"; }
  static std::string symbol() { return "meV"; }
};

template <> struct boost::units::base_unit_info<datasetunits::tof_base_unit> {
  static std::string name() { return "microseconds"; }
  static std::string symbol() { return "us"; }
};

class Unit {
public:
  // Define a std::variant which will hold the set of allowed units.
  // Any unit that does not exist in the variant will either fail to compile or
  // throw a std::runtime_error during operations such as multiplication or
  // division.
  // 
  // TODO: maybe it is possible to create a helper that will automatically
  // generate the squares for variance?
  // The following was attempted but did not succeed:
  //  template <class... Ts> struct unit_and_variance {
  //  using type =
  //    std::variant<Ts..., decltype(std::declval<Ts>()*std::declval<Ts>())...>;
  //  };
  typedef std::variant<
      // Dimensionless [ ]
      boost::units::si::dimensionless,
      // Length [m]
      boost::units::si::length,
      // Area [m^2]
      boost::units::si::area,
      // Time [s]
      boost::units::si::time,
      // Mass [kg]
      boost::units::si::mass,
      // Counts [counts]
      datasetunits::counts,
      // InverseLength [m^-1]
      decltype(std::declval<boost::units::si::dimensionless>() /
               std::declval<boost::units::si::length>()),
      // Wavelength [Angstroms]
      datasetunits::wavelength,
      // Energy [meV]
      datasetunits::energy,
      // Time of flight [microseconds]
      datasetunits::tof,
      // 1/time [s^-1]
      decltype(std::declval<boost::units::si::dimensionless>() /
               std::declval<boost::units::si::time>()),
      // Area variance
      decltype(std::declval<boost::units::si::area>() *
               std::declval<boost::units::si::area>()),
      // Counts variance
      decltype(std::declval<datasetunits::counts>() *
               std::declval<datasetunits::counts>())>
      unit_t;

  enum class Id : uint16_t {
    Dimensionless,
    Length,
    Area,
    AreaVariance,
    Counts,
    CountsVariance,
    InverseLength,
    Energy,
    Wavelength,
    Time,
    Tof,
    Mass
  };
  // TODO should this be explicit?
  Unit() = default;
  // TODO: should we have a templated constructor here?
  // e.g.: template <class T> Unit(T &&unit) : m_unit(std::forward<T>(unit)) {}
  Unit(const Unit::Id id);
  Unit(const unit_t unit) : m_unit(unit) {}

  Unit::Id id() const;
  const Unit::unit_t &getUnit() const { return m_unit; }

private:
  unit_t m_unit;
  // TODO need to support scale
};

inline bool operator==(const Unit &a, const Unit &b) {
  return a.id() == b.id();
}
inline bool operator!=(const Unit &a, const Unit &b) { return !(a == b); }

Unit operator+(const Unit &a, const Unit &b);
Unit operator*(const Unit &a, const Unit &b);
Unit operator/(const Unit &a, const Unit &b);

#endif // UNIT_H
