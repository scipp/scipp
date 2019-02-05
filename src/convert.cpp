/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>

#include "convert.h"
#include "dataset.h"
#include "md_zip_view.h"

namespace neutron {
namespace tof {
Dataset tofToEnergy(const Dataset &d) {
  // TODO Could in principle also support inelastic. Note that the conversion in
  // Mantid is wrong since it handles inelastic data as if it were elastic.
  if (d.contains(Coord::Ei{}) || d.contains(Coord::Ef{}))
    throw std::runtime_error("Dataset contains Coord::Ei or Coord::Ef. "
                             "However, conversion to Dim::Energy is currently "
                             "only supported for elastic scattering.");

  // 1. Compute conversion factor
  const auto &compPos = d.get(Coord::ComponentInfo{})[0].get(Coord::Position{});
  // TODO Need a better mechanism to identify source and sample.
  const auto &sourcePos = compPos[0];
  const auto &samplePos = compPos[1];
  const double l1 = (sourcePos - samplePos).norm();

  const auto &dims = d.contains(Coord::Position{})
                         ? d(Coord::Position{}).dimensions()
                         : d(Coord::DetectorGrouping{}).dimensions();
  Variable conversionFactor(Data::Value{}, dims);

  // This scale factor should be obtained from the unit stored in the Tof
  // coordinate, something like mus2_to_s2 = pow(tof.unit().si_scale(), 2);
  const double mus2_to_s2 = 1e-12;
  // Can we obtain this scale from the runtime unit? It contains joule, so it is
  // not so obvious how to do this.
  const auto meV = 1e-3 * boost::units::si::constants::codata::e.value() *
                   boost::units::si::joule;
  auto physicalConstants =
      boost::units::si::constants::codata::m_n / (2.0 * meV * mus2_to_s2);

  auto specPos = zipMD(d, MDRead(Coord::Position{}));
  const auto factor = [&](const auto &item) {
    const auto &pos = item.template get<Coord::Position>();
    double l_total = l1 + (samplePos - pos).norm();
    return l_total * l_total * physicalConstants.value();
  };

  // TODO Must also update unit of conversionFactor.
  // convertFactor.unit() *= extractUnit(physicalConstants);
  std::transform(specPos.begin(), specPos.end(),
                 conversionFactor.span<double>().begin(), factor);

  // 2. Transform variables
  Dataset converted;
  for (const auto &var : d) {
    auto varDims = var.dimensions();
    if (varDims.contains(Dim::Tof))
      varDims.relabel(varDims.index(Dim::Tof), Dim::Energy);
    if (var.tag() == Coord::Tof{}) {
      // TODO Need to extend the broadcasting capabilities to broadcast to the
      // union of dimensions of both operands in a binary operation.
      auto dims = conversionFactor.dimensions();
      for (const Dim dim : varDims.labels())
        if (!dims.contains(dim))
          dims.addInner(dim, varDims[dim]);
      // TODO Should have a broadcasting assign method?
      Variable energy(Coord::Energy{}, dims, dims.volume(), 1.0);
      energy *= conversionFactor;
      // The reshape is just to remap the dimension label, should probably do
      // this differently.
      energy /= (var * var).reshape(varDims);
      converted.insert(energy);
    } else if (var.tag() == Data::Events{}) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else {
      // Changing Dim::Tof to Dim::Energy.
      // TODO Also need to check here if variable contains count/bin_width,
      // should fail then?
      converted.insert(var.reshape(varDims));
    }
  }

  return converted;
}

Dataset tofToDeltaE(const Dataset &d) {
  // TODO Units and physical constants!

  // There are two cases, direct inelastic and indirect inelastic. We can
  // distinguish them by the content of d.
  if (d.contains(Coord::Ei{}) && d.contains(Coord::Ef{}))
    throw std::runtime_error("Dataset contains Coord::Ei as well as Coord::Ef, "
                             "cannot have both for inelastic scattering.");

  // 1. Compute conversion factors
  const auto &compPos = d.get(Coord::ComponentInfo{})[0].get(Coord::Position{});
  const auto &sourcePos = compPos[0];
  const auto &samplePos = compPos[1];
  const double l1 = (sourcePos - samplePos).norm();

  const auto &dims = d.contains(Coord::Position{})
                         ? d(Coord::Position{}).dimensions()
                         : d(Coord::DetectorGrouping{}).dimensions();
  Variable tofShift(Data::Value{}, {});
  Variable scale(Data::Value{}, {});

  if (d.contains(Coord::Ei{})) {
    // Direct-inelastic.

    // This is how we support multi-Ei data!
    tofShift.setDimensions(d(Coord::Ei{}).dimensions());
    const auto &Ei = d.get(Coord::Ei{});
    std::transform(Ei.begin(), Ei.end(), tofShift.span<double>().begin(),
                   [&l1](const double Ei) { return l1 / sqrt(Ei); });

    scale.setDimensions(dims);
    auto specPos = zipMD(d, MDRead(Coord::Position{}));
    std::transform(specPos.begin(), specPos.end(), scale.span<double>().begin(),
                   [&](const auto &item) {
                     const auto &pos = item.template get<Coord::Position>();
                     const double l2 = (samplePos - pos).norm();
                     return l2 * l2;
                   });
  } else if (d.contains(Coord::Ef{})) {
    // Indirect-inelastic.

    tofShift.setDimensions(dims);
    // Ef can be different for every spectrum so we access it also via a view.
    auto geometry = zipMD(d, MDRead(Coord::Position{}), MDRead(Coord::Ef{}));
    std::transform(geometry.begin(), geometry.end(),
                   tofShift.span<double>().begin(), [&](const auto &item) {
                     const auto &pos = item.template get<Coord::Position>();
                     const auto &Ef = item.template get<Coord::Ef>();
                     const double l2 = (samplePos - pos).norm();
                     return l2 * l2 / sqrt(Ef);
                   });

    scale.setDimensions({});
    scale.span<double>()[0] = l1 * l1;
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
    if (var.tag() == Coord::Tof{}) {
      auto dims = scale.dimensions();
      for (const Dim dim : varDims.labels())
        if (!dims.contains(dim))
          dims.addInner(dim, varDims[dim]);
      Variable E(Coord::DeltaE{}, dims, dims.volume(), 1.0);
      E *= var.reshape(varDims);
      E -= tofShift;
      E *= E;
      E = 1.0 / std::move(E);
      E *= scale;
      if (d.contains(Coord::Ei{})) {
        converted.insert(-(std::move(E) - d(Coord::Ei{})));
      } else {
        converted.insert(std::move(E) - d(Coord::Ef{}));
      }
    } else if (var.tag() == Data::Events{}) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else {
      converted.insert(var.reshape(varDims));
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
