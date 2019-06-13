// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/counts.h"
#include "dataset.h"
#include "except.h"

namespace scipp::core {

namespace counts {

auto getBinWidths(const Dataset &d, const std::vector<Dim> &dims) {
  std::vector<Variable> binWidths;
  for (const auto dim : dims) {
    const auto &coord = d.coords()[dim];
    if (coord.unit() == units::dimensionless)
      throw std::runtime_error("Dimensionless axis cannot be used for "
                               "conversion from or to density");
    binWidths.emplace_back(coord.slice({dim, 1, coord.dims()[dim]}) -
                           coord.slice({dim, 0, coord.dims()[dim] - 1}));
  }
  return binWidths;
}

void toDensity(const DataProxy data, const std::vector<Variable> &binWidths) {
  if (data.unit().isCounts()) {
    for (const auto &binWidth : binWidths)
      data /= binWidth;
  } else if (data.unit().isCountDensity()) {
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

Dataset toDensity(Dataset d, const Dim dim) {
  return toDensity(std::move(d), std::vector<Dim>{dim});
}

Dataset toDensity(Dataset d, const std::vector<Dim> &dims) {
  const auto binWidths = getBinWidths(d, dims);
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    toDensity(data, binWidths);
  }
  return std::move(d);
}

void fromDensity(const DataProxy data, const std::vector<Variable> &binWidths) {
  if (data.unit().isCounts()) {
    // Do nothing, but do not fail either.
  } else if (data.unit().isCountDensity()) {
    for (const auto &binWidth : binWidths)
      data *= binWidth;
  }
}

Dataset fromDensity(Dataset d, const Dim dim) {
  return fromDensity(std::move(d), std::vector<Dim>{dim});
}

Dataset fromDensity(Dataset d, const std::vector<Dim> &dims) {
  const auto binWidths = getBinWidths(d, dims);
  for (const auto & [ name, data ] : d) {
    static_cast<void>(name);
    fromDensity(data, binWidths);
  }
  return std::move(d);
}

} // namespace counts
} // namespace scipp::core
