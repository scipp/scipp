// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#ifndef UNIT_H
#define UNIT_H

#include <variant>

#include <boost/units/base_dimension.hpp>
#include <boost/units/make_system.hpp>
#include <boost/units/systems/si.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>
#include <boost/units/unit.hpp>

namespace scipp::units {
namespace neutron {
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
} // namespace neutron
} // namespace scipp::units

// Define conversion factors: the conversion will work both ways with a single
// macro

// Convert angstroms to SI meters
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    scipp::units::neutron::tof::wavelength_base_unit, boost::units::si::length,
    double, 1.0e-10);
// Convert meV to SI Joule
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    scipp::units::neutron::tof::energy_base_unit, boost::units::si::energy,
    double, 1.0e-3 * boost::units::si::constants::codata::e.value().value());
// Convert tof microseconds to SI seconds
BOOST_UNITS_DEFINE_CONVERSION_FACTOR(scipp::units::neutron::tof::tof_base_unit,
                                     boost::units::si::time, double, 1.0e-6);

BOOST_UNITS_DEFINE_CONVERSION_FACTOR(
    scipp::units::neutron::tof::velocity_base_unit, boost::units::si::velocity,
    double, boost::units::si::constants::codata::c.value().value());

// Define full and short names for the new units
template <>
struct boost::units::base_unit_info<
    scipp::units::neutron::tof::wavelength_base_unit> {
  static std::string name() { return "angstroms"; }
  static std::string symbol() { return "\u212B"; }
};

template <>
struct boost::units::base_unit_info<
    scipp::units::neutron::tof::counts_base_unit> {
  static std::string name() { return "counts"; }
  static std::string symbol() { return "counts"; }
};

template <>
struct boost::units::base_unit_info<
    scipp::units::neutron::tof::energy_base_unit> {
  static std::string name() { return "milli-electronvolt"; }
  static std::string symbol() { return "meV"; }
};

template <>
struct boost::units::base_unit_info<scipp::units::neutron::tof::tof_base_unit> {
  static std::string name() { return "microseconds"; }
  static std::string symbol() { return "\u03BCs"; }
};

template <>
struct boost::units::base_unit_info<
    scipp::units::neutron::tof::velocity_base_unit> {
  static std::string name() { return "c"; }
  static std::string symbol() { return "c"; }
};

namespace scipp::units {
// Helper variables to make the declaration units more succinct.
static constexpr boost::units::si::dimensionless dimensionless;
static constexpr boost::units::si::length m;
static constexpr boost::units::si::time s;
static constexpr boost::units::si::mass kg;
static constexpr boost::units::si::temperature K;
// Note the factor `dimensionless` in units that otherwise contain only non-SI
// factors. This is a trick to overcome some subtleties of working with
// heterogeneous unit systems in boost::units: We are combing SI units with our
// own, and the two are considered independent unless you convert explicitly.
// Therefore, in operations like (counts * m) / m, boosts is not cancelling the
// m as expected
// --- you get counts * dimensionless. Explicitly putting a factor dimensionless
// (dimensionless) into all our non-SI units avoids special-case handling in all
// operations (which would attempt to remove the dimensionless factor manually).
static constexpr decltype(neutron::tof::counts{} * dimensionless) counts;
static constexpr decltype(neutron::tof::wavelength{} * dimensionless) angstrom;
static constexpr decltype(neutron::tof::energy{} * dimensionless) meV;
static constexpr decltype(neutron::tof::tof{} * dimensionless) us;
static constexpr decltype(neutron::tof::velocity{} * dimensionless) c;

// Define a std::variant which will hold the set of allowed units. Any unit that
// does not exist in the variant will either fail to compile or throw a
// std::runtime_error during operations such as multiplication or division.
namespace detail {
template <class... Ts, class... Extra>
std::variant<Ts...,
             decltype(std::declval<std::remove_cv_t<Ts>>() *
                      std::declval<std::remove_cv_t<Ts>>())...,
             std::remove_cv_t<Extra>...>
make_unit(const std::tuple<Ts...> &, const std::tuple<Extra...> &) {
  return {};
}
} // namespace detail

template <class T> class Unit_impl {
public:
  using unit_t = T;

  constexpr Unit_impl() = default;
  // TODO should this be explicit?
  template <class Dim, class System, class Enable>
  Unit_impl(boost::units::unit<Dim, System, Enable> unit) : m_unit(unit) {}
  explicit Unit_impl(const unit_t &unit) : m_unit(unit) {}

  constexpr const Unit_impl::unit_t &operator()() const noexcept {
    return m_unit;
  }

  std::string name() const;

  bool operator==(const Unit_impl<T> &other) const;
  bool operator!=(const Unit_impl<T> &other) const;

private:
  unit_t m_unit;
  // TODO need to support scale
};

template <class T>
Unit_impl<T> operator+(const Unit_impl<T> &a, const Unit_impl<T> &b);
template <class T>
Unit_impl<T> operator-(const Unit_impl<T> &a, const Unit_impl<T> &b);
template <class T>
Unit_impl<T> operator*(const Unit_impl<T> &a, const Unit_impl<T> &b);
template <class T>
Unit_impl<T> operator/(const Unit_impl<T> &a, const Unit_impl<T> &b);
template <class T> Unit_impl<T> sqrt(const Unit_impl<T> &a);

template <class T> bool containsCounts(const Unit_impl<T> &unit);

namespace neutron {
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

#endif // UNIT_H
