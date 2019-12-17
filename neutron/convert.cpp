// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>

#include "scipp/core/dataset.h"
#include "scipp/core/dataset_util.h"
#include "scipp/core/transform.h"
#include "scipp/neutron/beamline.h"
#include "scipp/neutron/convert.h"

using namespace scipp::core;

namespace scipp::neutron {

const auto tof_to_s =
    boost::units::quantity<boost::units::si::time>(1.0 * units::us) / units::us;
const auto J_to_meV =
    units::meV /
    boost::units::quantity<boost::units::si::energy>(1.0 * units::meV);
const auto m_to_angstrom =
    units::angstrom /
    boost::units::quantity<boost::units::si::length>(1.0 * units::angstrom);
// In tof-to-energy conversions we *divide* by time-of-flight (squared), so the
// tof_to_s factor is in the denominator.
const auto tofToEnergyPhysicalConstants =
    0.5 * boost::units::si::constants::codata::m_n * J_to_meV /
    (tof_to_s * tof_to_s);

template <class T>
static T convert_with_factor(T &&d, const Dim from, const Dim to,
                             const Variable &factor) {
  // 1. Transform coordinate
  // Cannot use *= since often a broadcast into Dim::Spectrum is required.
  if (d.coords().contains(from)) {
    const auto &coords = d.coords()[from];
    d.setCoord(from, coords * astype(factor, coords.dtype()));
  }

  // 2. Transform sparse coordinates
  for (const auto &[name, data] : iter(d)) {
    static_cast<void>(name);
    if (data.dims().sparse()) {
      data.coords()[from] *= factor;
    }
  }
  d.rename(from, to);
  return std::move(d);
}

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

  conversionFactor *= 2.0 * boost::units::si::constants::codata::m_n /
                      boost::units::si::constants::codata::h /
                      (m_to_angstrom * tof_to_s);
  conversionFactor *= sqrt(0.5 * (1.0 - dot(beam, scattered)));

  return reciprocal(conversionFactor);
}

template <class T> static auto tofToWavelength(const T &d) {
  return (tof_to_s * m_to_angstrom * boost::units::si::constants::codata::h /
          boost::units::si::constants::codata::m_n) /
         neutron::flight_path_length(d);
}

template <class T> auto tofToEnergyConversionFactor(const T &d) {
  // l_total = l1 + l2
  auto conversionFactor = neutron::flight_path_length(d);
  // l_total^2
  conversionFactor *= conversionFactor;

  conversionFactor *= tofToEnergyPhysicalConstants;

  return conversionFactor;
}

template <class T> T tofToEnergy(T &&d) {
  // 1. Compute conversion factor
  const auto conversionFactor = tofToEnergyConversionFactor(d);

  // 2. Transform coordinate
  if (d.coords().contains(Dim::Tof)) {
    const auto &coordSq = d.coords()[Dim::Tof] * d.coords()[Dim::Tof];
    d.setCoord(Dim::Tof, (reciprocal(coordSq) *
                          astype(conversionFactor, coordSq.dtype())));
  }

  // 3. Transform sparse coordinates
  for (const auto &[name, data] : iter(d)) {
    static_cast<void>(name);
    if (data.coords()[Dim::Tof].dims().sparse()) {
      transform_in_place<pair_self_t<double, float>>(
          data.coords()[Dim::Tof], conversionFactor,
          [](auto &coord_, const auto &factor) {
            coord_ = factor / (coord_ * coord_);
          });
    }
  }

  d.rename(Dim::Tof, Dim::Energy);
  return std::move(d);
}

template <class T> T energyToTof(T &&d) {
  // 1. Compute conversion factor
  const auto conversionFactor = tofToEnergyConversionFactor(d);

  // 2. Transform coordinate
  if (d.coords().contains(Dim::Energy)) {
    const auto &coordSqrt = d.coords()[Dim::Energy];
    d.setCoord(Dim::Energy,
               sqrt(astype(conversionFactor, coordSqrt.dtype()) / coordSqrt));
  }

  // 3. Transform sparse coordinates
  for (const auto &[name, data] : iter(d)) {
    static_cast<void>(name);
    if (data.coords()[Dim::Energy].dims().sparse()) {
      transform_in_place<pair_self_t<double, float>>(
          data.coords()[Dim::Energy], conversionFactor,
          [](auto &coord_, const auto &factor) {
            coord_ = sqrt(factor / coord_);
          });
    }
  }

  d.rename(Dim::Energy, Dim::Tof);
  return std::move(d);
}

/*
Dataset tofToDeltaE(const Dataset &d) {
  // There are two cases, direct inelastic and indirect inelastic. We can
  // distinguish them by the content of d.
  if (d.contains(Coord::Ei) && d.contains(Coord::Ef))
    throw std::runtime_error("Dataset contains Coord::Ei as well as Coord::Ef, "
                             "cannot have both for inelastic scattering.");

  // 1. Compute conversion factors
  const auto &compPos = d.get(Coord::ComponentInfo)[0](Coord::Position);
  const auto &sourcePos = compPos(Dim::Component, 0);
  const auto &samplePos = compPos(Dim::Component, 1);
  auto l1_square = norm(sourcePos - samplePos);
  l1_square *= l1_square;
  l1_square *= tofToEnergyPhysicalConstants;
  const auto specPos = getSpecPos(d);
  auto l2_square = norm(specPos - samplePos);
  l2_square *= l2_square;
  l2_square *= tofToEnergyPhysicalConstants;

  auto tofShift = makeVariable<double>({});
  auto scale = makeVariable<double>({});

  if (d.contains(Coord::Ei)) {
    // Direct-inelastic.
    // This is how we support multi-Ei data!
    tofShift = sqrt(l1_square / d(Coord::Ei));
    scale = std::move(l2_square);
  } else if (d.contains(Coord::Ef)) {
    // Indirect-inelastic.
    // Ef can be different for every spectrum.
    tofShift = sqrt(std::move(l2_square) / d(Coord::Ef));
    scale = std::move(l1_square);
  } else {
    throw std::runtime_error("Dataset contains neither Coord::Ei nor "
                             "Coord::Ef, this does not look like "
                             "inelastic-scattering data.");
  }

  // 2. Transform variables
  Dataset converted;
  for (const auto & [ name, tag, var ] : d) {
    auto varDims = var.dimensions();
    if (varDims.contains(Dim::Tof))
      varDims.relabel(varDims.index(Dim::Tof), Dim::DeltaE);
    if (tag == Coord::Tof) {
      Variable inv_tof = 1.0 / (var.reshape(varDims) - tofShift);
      Variable E = inv_tof * inv_tof * scale;
      if (d.contains(Coord::Ei)) {
        converted.insert(Coord::DeltaE, -(std::move(E) - d(Coord::Ei)));
      } else {
        converted.insert(Coord::DeltaE, std::move(E) - d(Coord::Ef));
      }
    } else if (tag == Data::Events) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else {
      if (counts::isDensity(var))
        throw std::runtime_error("TODO Converting units of count-density data "
                                 "not implemented yet for this case.");
      converted.insert(tag, name, var.reshape(varDims));
    }
  }

  // TODO Do we always require reversing for inelastic?
  // TODO Is is debatable whether this should revert automatically... probably
  // not, but we need to put a check in place for `rebin` to fail if the axis is
  // reversed.
  return reverse(converted, Dim::DeltaE);
}
*/

template <class T> T convert_impl(T d, const Dim from, const Dim to) {
  for (const auto &item : iter(d))
    if (item.second.hasData())
      expect::notCountDensity(item.second.unit());
  if ((from == Dim::Tof) && (to == Dim::DSpacing))
    return convert_with_factor(std::move(d), from, to, tofToDSpacing(d));
  if ((from == Dim::DSpacing) && (to == Dim::Tof))
    return convert_with_factor(std::move(d), from, to,
                               reciprocal(tofToDSpacing(d)));

  if ((from == Dim::Tof) && (to == Dim::Wavelength))
    return convert_with_factor(std::move(d), from, to, tofToWavelength(d));
  if ((from == Dim::Wavelength) && (to == Dim::Tof))
    return convert_with_factor(std::move(d), from, to,
                               reciprocal(tofToWavelength(d)));

  if ((from == Dim::Tof) && (to == Dim::Energy))
    return tofToEnergy(std::move(d));
  if ((from == Dim::Energy) && (to == Dim::Tof))
    return energyToTof(std::move(d));
  throw std::runtime_error(
      "Conversion between requested dimensions not implemented yet.");
}

DataArray convert(DataArray d, const Dim from, const Dim to) {
  return convert_impl(std::move(d), from, to);
}

DataArray convert(const DataConstProxy &d, const Dim from, const Dim to) {
  return convert(DataArray(d), from, to);
}

Dataset convert(Dataset d, const Dim from, const Dim to) {
  return convert_impl(std::move(d), from, to);
}

Dataset convert(const DatasetConstProxy &d, const Dim from, const Dim to) {
  return convert(Dataset(d), from, to);
}

} // namespace scipp::neutron
