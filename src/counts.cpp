/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "counts.h"
#include "dataset.h"

namespace counts {
Dataset toDensity(Dataset d, const Dim dim) {
  const auto &coord = d(dimensionCoord(dim));
  if (coord.unit() == Unit::Id::Dimensionless)
    throw std::runtime_error(
        "Dimensionless axis cannot be used for conversion to density");
  auto binWidths = coord(dim, 1, coord.dimensions()[dim]) -
                   coord(dim, 0, coord.dimensions()[dim] - 1);
  for(const auto &var : d) {
    if (var.isData()) {
      // TODO Should we fail if there are variables that are already
      // count-densities?
      if (var.unit() == Unit::Id::Counts) {
        var /= binWidths;
      } else if(var.unit() == Unit::Id::CountsVariance) {
        var /= binWidths * binWidths;
      }
    }
  }
  return std::move(d);
}

Dataset fromDensity(Dataset d, const Dim dim) {
  const auto &coord = d(dimensionCoord(dim));
  if (coord.unit() == Unit::Id::Dimensionless)
    throw std::runtime_error(
        "Dimensionless axis cannot be used for conversion from density");
  auto binWidths = coord(dim, 1, coord.dimensions()[dim]) -
                   coord(dim, 0, coord.dimensions()[dim] - 1);
  for(const auto &var : d) {
    if (var.isData()) {
      if (var.unit() == Unit::Id::Counts) {
        // TODO Already counts, ignore silently? See also toDensity.
        var /= binWidths;
      } else if(units::containsCounts(var.unit())) {
        var *= binWidths;
      } else if(units::containsCountsVariance(var.unit())) {
        var *= binWidths * binWidths;
      }
    }
  }
  return std::move(d);
}
} // namespace counts
