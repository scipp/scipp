// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>

#include "scipp/common/constants.h"

#include "scipp/units/unit.h"

#include "scipp/variable/operations.h"

#include "scipp/neutron/beamline.h"

namespace scipp::neutron {

namespace constants {

const auto tof_to_s = boost::units::quantity<boost::units::si::time>(
                          1.0 * units::boost_units::us) /
                      units::boost_units::us;
const auto J_to_meV =
    units::boost_units::meV / boost::units::quantity<boost::units::si::energy>(
                                  1.0 * units::boost_units::meV);
const auto m_to_angstrom = units::boost_units::angstrom /
                           boost::units::quantity<boost::units::si::length>(
                               1.0 * units::boost_units::angstrom);
// In tof-to-energy conversions we *divide* by time-of-flight (squared), so the
// tof_to_s factor is in the denominator.
const auto tofToEnergyPhysicalConstants =
    0.5 * boost::units::si::constants::codata::m_n * J_to_meV /
    (tof_to_s * tof_to_s);

template <class T> auto tofToDSpacing(const T &d) {
  const auto &sourcePos = source_position(d);
  const auto &samplePos = sample_position(d);

  auto beam = samplePos - sourcePos;
  const auto l1 = norm(beam);
  beam /= l1;
  auto scattered = neutron::position(d) - samplePos;
  const auto l2 = norm(scattered);
  scattered /= l2;

  // l_total = l1 + l2
  auto conversionFactor(l1 + l2);

  conversionFactor *= Variable(2.0 * boost::units::si::constants::codata::m_n /
                               boost::units::si::constants::codata::h /
                               (m_to_angstrom * tof_to_s));
  conversionFactor *=
      sqrt(0.5 * units::one * (1.0 * units::one - dot(beam, scattered)));

  return reciprocal(conversionFactor);
}

template <class T> static auto tofToWavelength(const T &d) {
  return Variable(tof_to_s * m_to_angstrom *
                  boost::units::si::constants::codata::h /
                  boost::units::si::constants::codata::m_n) /
         neutron::flight_path_length(d);
}

template <class T> auto tofToEnergy(const T &d) {
  // l_total = l1 + l2
  auto conversionFactor = neutron::flight_path_length(d);
  // l_total^2
  conversionFactor *= conversionFactor;
  conversionFactor *= Variable(tofToEnergyPhysicalConstants);
  return conversionFactor;
}

template <class T> auto wavelengthToQ(const T &d) {
  return sin(neutron::scattering_angle(d)) *
         (4.0 * scipp::pi<double> * units::one);
}

} // namespace constants

} // namespace scipp::neutron
