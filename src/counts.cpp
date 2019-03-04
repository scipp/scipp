/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "counts.h"
#include "dataset.h"
#include "except.h"

namespace counts {

auto getBinWidths(const Dataset &d, const std::initializer_list<Dim> &dims) {
  std::vector<Variable> binWidths;
  for (const auto dim : dims) {
    const auto &coord = d(dimensionCoord(dim));
    if (coord.unit() == units::dimensionless)
      throw std::runtime_error("Dimensionless axis cannot be used for "
                               "conversion from or to density");
    binWidths.emplace_back(coord(dim, 1, coord.dimensions()[dim]) -
                           coord(dim, 0, coord.dimensions()[dim] - 1));
  }
  return binWidths;
}

void toDensity(const VariableSlice var,
               const std::vector<Variable> &binWidths) {
  if (var.isData()) {
    if (var.unit() == units::counts) {
      for (const auto &binWidth : binWidths)
        var /= binWidth;
    } else if (var.unit() == units::counts * units::counts) {
      for (const auto &binWidth : binWidths)
        var /= binWidth * binWidth;
    } else if (units::containsCounts(var.unit())) {
      // This error implies that conversion to multi-dimensional densities
      // must be done in one step, e.g., counts -> counts/(m*m*s). We cannot
      // do counts -> counts/m -> counts/(m*m) -> counts/(m*m*s) since the
      // unit-based distinction between counts and counts-density cannot tell
      // apart different length dimensions such as X and Y, so we would not be
      // able to prevent converting to density using Dim::X twice.
      throw std::runtime_error("Cannot convert counts-variable to density, "
                               "it looks like it has already been "
                               "converted.");
    }
    // No `else`, variables that do not contain a `counts` factor are left
    // unchanged.
  }
}

Dataset toDensity(Dataset d, const Dim dim) {
  return toDensity(std::move(d), {dim});
}

Dataset toDensity(Dataset d, const std::initializer_list<Dim> &dims) {
  const auto binWidths = getBinWidths(d, dims);
  for (const auto &var : d)
    toDensity(var, binWidths);
  return std::move(d);
}

void fromDensity(const VariableSlice var,
                 const std::vector<Variable> &binWidths) {
  if (var.isData()) {
    if (var.unit() == units::counts) {
      // Do nothing, but do not fail either.
    } else if (units::containsCounts(var.unit())) {
      for (const auto &binWidth : binWidths)
        var *= binWidth;
      dataset::expect::unit(var, units::counts);
    } else if (units::containsCountsVariance(var.unit())) {
      for (const auto &binWidth : binWidths)
        var *= binWidth * binWidth;
      dataset::expect::unit(var, units::counts * units::counts);
    }
  }
}

Dataset fromDensity(Dataset d, const Dim dim) {
  return fromDensity(std::move(d), {dim});
}

Dataset fromDensity(Dataset d, const std::initializer_list<Dim> &dims) {
  const auto binWidths = getBinWidths(d, dims);
  for (const auto &var : d)
    fromDensity(var, binWidths);
  return std::move(d);
}

/// Returns true if the data in the variable is a counts-density.
//
// Note that we cannot distiguish between densities for different dimensions,
// since our unit system does not provide means to distinguish, e.g., meter for
// dimension X and meter for dimension Y.
bool isDensity(const Variable &var) {
  const auto &unit = var.unit();
  if (units::containsCounts(unit) && unit != units::counts)
    return true;
  if (units::containsCountsVariance(unit) &&
      unit != units::counts * units::counts)
    return true;
  return false;
}

} // namespace counts
