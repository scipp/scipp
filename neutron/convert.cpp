// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>

#include "scipp/core/counts.h"
#include "scipp/core/dataset.h"
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
    boost::units::quantity<boost::units::si::length>(1.0 * units::angstrom) /
    units::angstrom;
// In tof-to-energy conversions we *divide* by time-of-flight (squared), so the
// tof_to_s factor is in the denominator.
const auto tofToEnergyPhysicalConstants =
    0.5 * boost::units::si::constants::codata::m_n * J_to_meV /
    (tof_to_s * tof_to_s);
const auto tofToDSpacingPhysicalConstants =
    2.0 * boost::units::si::constants::codata::m_n * m_to_angstrom /
    (boost::units::si::constants::codata::h * tof_to_s);

auto computeTofToDSpacingConversionFactor(const Dataset &d) {
  const auto &sourcePos = source_position(d);
  const auto &samplePos = sample_position(d);

  auto beam = samplePos - sourcePos;
  const auto l1 = norm(beam);
  beam /= l1;
  auto scattered = d.coords()[Dim::Position] - samplePos;
  const auto l2 = norm(scattered);
  scattered /= l2;

  // l_total = l1 + l2
  auto conversionFactor(l1 + l2);

  conversionFactor *= tofToDSpacingPhysicalConstants;
  conversionFactor *= sqrt(0.5 * (1.0 - dot(beam, scattered)));

  return conversionFactor;
}

Dataset tofToDSpacing(Dataset &&d) {
  // 1. Compute conversion factor
  const auto conversionFactor = computeTofToDSpacingConversionFactor(d);

  // 2. Transform coordinate
  // Cannot use /= since often a broadcast into Dim::Position is required.
  d.setCoord(Dim::Tof, d.coords()[Dim::Tof] / conversionFactor);

  // 3. Transform variables
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    if (data.coords()[Dim::Tof].dims().sparse()) {
      data.coords()[Dim::Tof] /= conversionFactor;
    } else if (data.unit().isCountDensity()) {
      // Tof to DSpacing is just a scale factor, so density transform is simple:
      data *= conversionFactor;
    }
  }

  d.rename(Dim::Tof, Dim::DSpacing);
  return std::move(d);
}

Dataset dSpacingToTof(Dataset &&d) {
  // 1. Compute conversion factor
  const auto conversionFactor = computeTofToDSpacingConversionFactor(d);

  // 2. Transform coordinate
  // Cannot use *= since often a broadcast into Dim::Position is required.
  d.setCoord(Dim::DSpacing, d.coords()[Dim::DSpacing] * conversionFactor);

  // 3. Transform variables
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    if (data.coords()[Dim::DSpacing].dims().sparse()) {
      data.coords()[Dim::DSpacing] *= conversionFactor;
    } else if (data.unit().isCountDensity()) {
      // DSpacing to Tof is just a scale factor, so density transform is simple:
      data /= conversionFactor;
    }
  }

  d.rename(Dim::DSpacing, Dim::Tof);
  return std::move(d);
}

auto tofToEnergyConversionFactor(const Dataset &d) {
  const auto &samplePos = sample_position(d);
  const auto l1 = neutron::l1(d);
  const auto specPos = d.coords()[Dim::Position];

  // l_total = l1 + l2
  auto conversionFactor(norm(specPos - samplePos) + l1);
  // l_total^2
  conversionFactor *= conversionFactor;

  conversionFactor *= tofToEnergyPhysicalConstants;

  return conversionFactor;
}

Dataset tofToEnergy(Dataset &&d) {
  // 1. Compute conversion factor
  const auto conversionFactor = tofToEnergyConversionFactor(d);

  // 2. Record ToF bin widths
  const auto oldBinWidths = counts::getBinWidths(d, {Dim::Tof});

  // 3. Transform coordinate
  d.setCoord(Dim::Tof, (1.0 / (d.coords()[Dim::Tof] * d.coords()[Dim::Tof])) *
                           conversionFactor);

  // 4. Record energy bin widths
  const auto newBinWidths = counts::getBinWidths(d, {Dim::Tof});

  // 5. Transform variables
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    if (data.coords()[Dim::Tof].dims().sparse()) {
      data.coords()[Dim::Tof].assign(
          (1.0 / (data.coords()[Dim::Tof] * data.coords()[Dim::Tof])) *
          conversionFactor);
    } else if (data.unit().isCountDensity()) {
      counts::fromDensity(data, oldBinWidths);
      counts::toDensity(data, newBinWidths);
    }
  }

  d.rename(Dim::Tof, Dim::Energy);
  return std::move(d);
}

Dataset energyToTof(Dataset &&d) {
  // 1. Compute conversion factor
  const auto conversionFactor = tofToEnergyConversionFactor(d);

  // 2. Record energy bin widths
  const auto oldBinWidths = counts::getBinWidths(d, {Dim::Energy});

  // 3. Transform coordinate
  d.setCoord(Dim::Energy, sqrt(conversionFactor / d.coords()[Dim::Energy]));

  // 4. Record ToF bin widths
  const auto newBinWidths = counts::getBinWidths(d, {Dim::Energy});

  // 5. Transform variables
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    if (data.coords()[Dim::Energy].dims().sparse()) {
      data.coords()[Dim::Energy].assign(
          sqrt(conversionFactor / data.coords()[Dim::Energy]));
    } else if (data.unit().isCountDensity()) {
      counts::fromDensity(data, oldBinWidths);
      counts::toDensity(data, newBinWidths);
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

Dataset convert(Dataset d, const Dim from, const Dim to) {
  if ((from == Dim::Tof) && (to == Dim::DSpacing))
    return tofToDSpacing(std::move(d));
  if ((from == Dim::DSpacing) && (to == Dim::Tof))
    return dSpacingToTof(std::move(d));
  if ((from == Dim::Tof) && (to == Dim::Energy))
    return tofToEnergy(std::move(d));
  if ((from == Dim::Energy) && (to == Dim::Tof))
    return energyToTof(std::move(d));
  /*
  if ((from == Dim::Tof) && (to == Dim::DeltaE))
   return tofToDeltaE(d);
   */
  throw std::runtime_error(
      "Conversion between requested dimensions not implemented yet.");
  // How to convert? There are several cases:
  // 1. Tof conversion as Mantid's ConvertUnits.
  // 2. Axis conversion as Mantid's ConvertSpectrumAxis.
  // 3. Conversion of multiple dimensions simultaneuously, e.g., to Q, which
  //    cannot be done here since it affects more than one input and output
  //    dimension. Should we have a variant that accepts a list of dimensions
  //    for input and output?
  // 4. Conversion from 1 to N or N to 1, e.g., Dim::Spectrum to X and Y pixel
  //    index.
  // Can Dim::Spectrum be converted to anything? Should we require a matching
  // coordinate when doing a conversion? This does not make sense:
  // auto converted = convert(dataset, Dim::Spectrum, Dim::Tof);
  // This does if we can lookup the TwoTheta, make axis here, or require it?
  // Should it do the reordering? Is sorting separately much less efficient?
  // Dim::Spectrum is discrete, Dim::TwoTheta is in principle contiguous. How to
  // handle that? Do we simply want to sort instead? Discrete->contiguous can be
  // handled by binning? Or is Dim::TwoTheta implicitly also discrete?
  // auto converted = convert(dataset, Dim::Spectrum, Dim::TwoTheta);
  // This is a *derived* coordinate, no need to store it explicitly? May even be
  // prevented?
  // MDZipView<const Coord::TwoTheta>(dataset);
}

} // namespace scipp::neutron
