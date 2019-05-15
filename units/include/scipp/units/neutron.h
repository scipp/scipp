// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#ifndef SCIPP_UNITS_NEUTRON_H
#define SCIPP_UNITS_NEUTRON_H

#include "scipp/units/unit.h"

#include <boost/units/base_dimension.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>

namespace scipp::units {
namespace detail {
namespace tof {

// Base dimension of counts
struct counts_base_dimension
    : boost::units::base_dimension<counts_base_dimension, 1> {};
using counts_dimension = counts_base_dimension::dimension_type;

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
struct velocity_base_unit
    : boost::units::base_unit<velocity_base_unit,
                              boost::units::velocity_dimension, 5> {};

// Create the units system using the make_system utility
typedef boost::units::make_system<wavelength_base_unit, counts_base_unit,
                                  energy_base_unit, tof_base_unit>::type
    units_system;
// Velocity unit [c] has to be in its own system, otherwise we get unwanted
// cancellations with [Angstrom] and [us]. Should [meV] also be part of this
// system?
typedef boost::units::make_system<velocity_base_unit>::type units_system2;

typedef boost::units::unit<counts_dimension, units_system> counts;
typedef boost::units::unit<boost::units::length_dimension, units_system>
    wavelength;
typedef boost::units::unit<boost::units::energy_dimension, units_system> energy;
typedef boost::units::unit<boost::units::time_dimension, units_system> tof;
typedef boost::units::unit<boost::units::velocity_dimension, units_system2>
    velocity;

} // namespace tof
} // namespace detail
} // namespace scipp::units

// Define conversion factors: the conversion will work both ways with a single
// macro

// Convert angstroms to SI meters
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    scipp::units::detail::tof::wavelength_base_unit, boost::units::si::length,
    double, 1.0e-10);
// Convert meV to SI Joule
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    scipp::units::detail::tof::energy_base_unit, boost::units::si::energy,
    double, 1.0e-3 * boost::units::si::constants::codata::e.value().value());
// Convert tof microseconds to SI seconds
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(scipp::units::detail::tof::tof_base_unit,
                                     boost::units::si::time, double, 1.0e-6);

BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    scipp::units::detail::tof::velocity_base_unit, boost::units::si::velocity,
    double, boost::units::si::constants::codata::c.value().value());

// Define full and short names for the new units
template <>
struct boost::units::base_unit_info<
    scipp::units::detail::tof::wavelength_base_unit> {
  static std::string name() { return "angstroms"; }
  static std::string symbol() { return "\u212B"; }
};

template <>
struct boost::units::base_unit_info<
    scipp::units::detail::tof::counts_base_unit> {
  static std::string name() { return "counts"; }
  static std::string symbol() { return "counts"; }
};

template <>
struct boost::units::base_unit_info<
    scipp::units::detail::tof::energy_base_unit> {
  static std::string name() { return "milli-electronvolt"; }
  static std::string symbol() { return "meV"; }
};

template <>
struct boost::units::base_unit_info<scipp::units::detail::tof::tof_base_unit> {
  static std::string name() { return "microseconds"; }
  static std::string symbol() { return "\u03BCs"; }
};

template <>
struct boost::units::base_unit_info<
    scipp::units::detail::tof::velocity_base_unit> {
  static std::string name() { return "c"; }
  static std::string symbol() { return "c"; }
};

namespace scipp::units {
inline namespace neutron {
// Additional helper constants beyond the SI base units.
// Note the factor `dimensionless` in units that otherwise contain only non-SI
// factors. This is a trick to overcome some subtleties of working with
// heterogeneous unit systems in boost::units: We are combing SI units with our
// own, and the two are considered independent unless you convert explicitly.
// Therefore, in operations like (counts * m) / m, boosts is not cancelling the
// m as expected
// --- you get counts * dimensionless. Explicitly putting a factor dimensionless
// (dimensionless) into all our non-SI units avoids special-case handling in all
// operations (which would attempt to remove the dimensionless factor manually).
static constexpr decltype(detail::tof::counts{} * dimensionless) counts;
static constexpr decltype(detail::tof::wavelength{} * dimensionless) angstrom;
static constexpr decltype(detail::tof::energy{} * dimensionless) meV;
static constexpr decltype(detail::tof::tof{} * dimensionless) us;
static constexpr decltype(detail::tof::velocity{} * dimensionless) c;

using supported_units = decltype(detail::make_unit(
    std::make_tuple(m, dimensionless / m),
    std::make_tuple(dimensionless, counts, s, kg, angstrom, meV, us,
                    dimensionless / us, dimensionless / s, counts / us,
                    counts / meV, m *m *m *m, meV *us *us / (m * m),
                    meV *us *us *dimensionless, kg *m / s, m / s, c, c *m,
                    meV / c, dimensionless / c, K, us / angstrom,
                    us / (m * angstrom))));

using Unit = Unit_impl<supported_units>;
} // namespace neutron
} // namespace scipp::units

#endif // SCIPP_UNITS_NEUTRON_H
