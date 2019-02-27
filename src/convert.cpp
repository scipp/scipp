/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>

#include "convert.h"
#include "counts.h"
#include "dataset.h"
#include "md_zip_view.h"
#include "zip_view.h"

Variable getSpecPos(const Dataset &d) {
  // TODO There should be a better way to extract the actual spectrum positions
  // as a variable.
  if (d.contains(Coord::Position))
    return d(Coord::Position);
  auto specPosView = zipMD(d, MDRead(Coord::Position));
  Variable specPos(Coord::Position, d(Coord::DetectorGrouping).dimensions());
  std::transform(specPosView.begin(), specPosView.end(),
                 specPos.get(Coord::Position).begin(),
                 [](const auto &item) { return item.get(Coord::Position); });
  return specPos;
}

const auto tof_to_s =
    boost::units::quantity<boost::units::si::time>(1.0 * units::us) / units::us;
const auto J_to_meV =
    units::meV /
    boost::units::quantity<boost::units::si::energy>(1.0 * units::meV);
// In tof-to-energy conversions we *divide* by time-of-flight (squared), so the
// tof_to_s factor is in the denominator.
const auto tofToEnergyPhysicalConstants =
    0.5 * boost::units::si::constants::codata::m_n * J_to_meV /
    (tof_to_s * tof_to_s);

namespace neutron {
namespace tof {
Dataset tofToEnergy(const Dataset &d) {
  // TODO Could in principle also support inelastic. Note that the conversion in
  // Mantid is wrong since it handles inelastic data as if it were elastic.
  if (d.contains(Coord::Ei) || d.contains(Coord::Ef))
    throw std::runtime_error("Dataset contains Coord::Ei or Coord::Ef. "
                             "However, conversion to Dim::Energy is currently "
                             "only supported for elastic scattering.");

  // 1. Compute conversion factor
  const auto &compPos = d.get(Coord::ComponentInfo)[0](Coord::Position);
  // TODO Need a better mechanism to identify source and sample.
  const auto &sourcePos = compPos(Dim::Component, 0);
  const auto &samplePos = compPos(Dim::Component, 1);
  const auto l1 = norm(sourcePos - samplePos);
  const auto specPos = getSpecPos(d);

  // l_total = l1 + l2
  auto conversionFactor(norm(specPos - samplePos) + l1);
  // l_total^2
  conversionFactor *= conversionFactor;

  conversionFactor *= tofToEnergyPhysicalConstants;

  // 2. Transform coordinate
  Dataset converted;
  const auto &coord = d(Coord::Tof);
  auto coordDims = coord.dimensions();
  coordDims.relabel(coordDims.index(Dim::Tof), Dim::Energy);
  // The reshape is to remap the dimension label, should probably be done
  // differently. Binary op order is to get desired dimension broadcast.
  Variable inv = 1.0 / (coord * coord).reshape(coordDims);
  converted.insert(Coord::Energy, std::move(inv) * conversionFactor);

  // 3. Transform variables
  for (const auto &var : d) {
    auto varDims = var.dimensions();
    if (varDims.contains(Dim::Tof))
      varDims.relabel(varDims.index(Dim::Tof), Dim::Energy);
    if (var.tag() == Coord::Tof) {
      // Done already.
    } else if (var.tag() == Data::Events) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else {
      // Changing Dim::Tof to Dim::Energy.
      if (::counts::isDensity(var)) {
        // The way of handling density data here looks less than optimal. We
        // either need to encapsulate this better or require manual conversion
        // from density before applying unit converions.
        const auto size = coord.dimensions()[Dim::Tof];
        const auto oldBinWidth =
            coord(Dim::Tof, 1, size) - coord(Dim::Tof, 0, size - 1);
        const auto &newCoord = converted(Coord::Energy);
        const auto newBinWidth =
            newCoord(Dim::Energy, 1, size) - newCoord(Dim::Energy, 0, size - 1);

        converted.insert(var);
        ::counts::fromDensity(converted(var.tag(), var.name()), {oldBinWidth});
        converted.insert(
            converted.erase(var.tag(), var.name()).reshape(varDims));
        ::counts::toDensity(converted(var.tag(), var.name()), {newBinWidth});
      } else {
        converted.insert(var.reshape(varDims));
      }
    }
  }

  return converted;
}

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

  Variable tofShift(Data::Value, {});
  Variable scale(Data::Value, {});

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
  for (const auto &var : d) {
    auto varDims = var.dimensions();
    if (varDims.contains(Dim::Tof))
      varDims.relabel(varDims.index(Dim::Tof), Dim::DeltaE);
    if (var.tag() == Coord::Tof) {
      Variable inv_tof = 1.0 / (var.reshape(varDims) - tofShift);
      Variable E = inv_tof * inv_tof * scale;
      if (d.contains(Coord::Ei)) {
        converted.insert(Coord::DeltaE, -(std::move(E) - d(Coord::Ei)));
      } else {
        converted.insert(Coord::DeltaE, std::move(E) - d(Coord::Ef));
      }
    } else if (var.tag() == Data::Events) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else {
      if (::counts::isDensity(var))
        throw std::runtime_error("TODO Converting units of count-density data "
                                 "not implemented yet for this case.");
      converted.insert(var.reshape(varDims));
    }
  }

  return converted;
}

gsl::index continuousToIndex(const double val,
                             const gsl::span<const double> axis) {
  const auto lower = std::lower_bound(axis.begin(), axis.end(), val);
  const auto upper = std::upper_bound(axis.begin(), axis.end(), val);
  if (upper == axis.end() || upper == axis.begin())
    return -1;
  return std::distance(axis.begin(), lower) - 1;
}

Dataset continuousToIndex(const Variable &values, const Dataset &coords) {
  const auto &vals = values.span<Eigen::Vector3d>();
  const auto &qx = coords.get(Coord::Qx);
  const auto &qy = coords.get(Coord::Qy);
  const auto &qz = coords.get(Coord::Qz);
  std::vector<gsl::index> ix;
  std::vector<gsl::index> iy;
  std::vector<gsl::index> iz;
  for (const auto &val : vals) {
    ix.push_back(continuousToIndex(val[0], qx));
    iy.push_back(continuousToIndex(val[1], qy));
    iz.push_back(continuousToIndex(val[2], qz));
  }
  Dataset index;
  index.insert<gsl::index>(Coord::Qx, values.dimensions(), ix);
  index.insert<gsl::index>(Coord::Qy, values.dimensions(), iy);
  index.insert<gsl::index>(Coord::Qz, values.dimensions(), iz);
  return index;
}

Dataset positionToQ(const Dataset &d, const Dataset &qCoords) {
  const auto &compPos = d.get(Coord::ComponentInfo)[0](Coord::Position);
  const auto &sourcePos = compPos(Dim::Component, 0);
  const auto &samplePos = compPos(Dim::Component, 1);
  const auto l1 = norm(sourcePos - samplePos);
  const auto specPos = getSpecPos(d);

  auto ki = samplePos - sourcePos;
  ki /= norm(ki);
  ki = ki * d(Coord::Ei) /* TODO c^-1 */;

  auto kf = specPos - samplePos;
  kf /= norm(kf);
  kf = kf * (d(Coord::Ei) + d(Coord::DeltaE)); // TODO sign?

  // ki has {Dim::Ei}
  // kf has {Dim::Ei, Dim::DeltaE, Dim::Position}
  // thus qIndex also has {Dim::Ei, Dim::DeltaE, Dim::Position}
  const auto Q = ki - kf;
  const auto qIndex = continuousToIndex(Q, qCoords);

  Dataset converted(qCoords);
  converted.erase(Coord::DeltaE);
  for (const auto &var : d) {
    if (var.tag() == Data::Events || var.tag() == Data::EventTofs) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else if (var.dimensions().contains(Dim::Position) &&
               var.dimensions().contains(Dim::DeltaE)) {
      // Position axis is converted into 3 Q axes.
      auto dims = var.dimensions();
      // TODO Make sure that Dim::Position is outer, otherwise insert
      // Q-dimensions correctly elsewhere.
      dims.erase(Dim::Position);
      dims.add(Dim::Qx, qCoords.dimensions()[Dim::Qx] - 1);
      dims.add(Dim::Qy, qCoords.dimensions()[Dim::Qy] - 1);
      dims.add(Dim::Qz, qCoords.dimensions()[Dim::Qz] - 1);

      Variable tmp(var, dims);

      for (gsl::index deltaE = 0; deltaE < var.dimensions()[Dim::DeltaE];
           ++deltaE) {
        const auto in = var(Dim::DeltaE, deltaE);
        const auto out = tmp(Dim::DeltaE, deltaE);
        const Dataset indices = qIndex(Dim::DeltaE, deltaE);
        const auto q = zip(indices, Access::Key<gsl::index>{Coord::Qx},
                           Access::Key<gsl::index>{Coord::Qy},
                           Access::Key<gsl::index>{Coord::Qz});
        if (in.dimensions()[Dim::Position] != q.size())
          throw std::runtime_error("Broken implementation of convert.");
        for (gsl::index i = 0; i < q.size(); ++i) {
          const auto[qx, qy, qz] = q[i];
          // Drop out-of-range values
          if (qx < 0 || qy < 0 || qz < 0)
            continue;
          // Really inefficient accumulation of volume histogram
          out(Dim::Qx, qx)(Dim::Qy, qy)(Dim::Qz, qz) += in(Dim::Position, i);
        }
      }
      converted.insert(std::move(tmp));
    } else if (var.dimensions().contains(Dim::Position)) {
      // TODO Drop?
    } else {
      converted.insert(var);
    }
  }

  return converted;
}

} // namespace tof
} // namespace neutron

Dataset convert(const Dataset &d, const Dim from, const Dim to) {
  if ((from == Dim::Tof) && (to == Dim::Energy))
    return neutron::tof::tofToEnergy(d);
  if ((from == Dim::Tof) && (to == Dim::DeltaE))
    return neutron::tof::tofToDeltaE(d);
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

bool contains(const std::vector<Dim> &dims, const Dim dim) {
  return std::find(dims.begin(), dims.end(), dim) != dims.end();
}

Dataset convert(const Dataset &d, const std::vector<Dim> &from,
                const Dataset &toCoords) {
  if (from.size() == 2 && contains(from, Dim::Position) &&
      contains(from, Dim::DeltaE)) {
    // Converting from position space
    if (toCoords.size() == 4 && toCoords.contains(Coord::DeltaE) &&
        toCoords.contains(Coord::Qx) && toCoords.contains(Coord::Qy) &&
        toCoords.contains(Coord::Qz)) {
      // Converting to momentum transfer
      if (d(Coord::DeltaE) != toCoords(Coord::DeltaE)) {
        // TODO Do we lose precision by rebinning before having computed Q?
        // Should we map to the output DeltaE only in the main conversion step?
        auto converted = rebin(d, toCoords(Coord::DeltaE));
        return neutron::tof::positionToQ(converted, toCoords);
      } else {
        return neutron::tof::positionToQ(d, toCoords);
      }
    }
  }
  throw std::runtime_error(
      "Conversion between requested dimensions not implemented yet.");
}
