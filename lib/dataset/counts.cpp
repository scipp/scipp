// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/operations.h"

#include "scipp/dataset/counts.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

namespace scipp::dataset {

namespace counts {

std::vector<Variable> getBinWidths(const Coords &c,
                                   const std::vector<Dim> &dims) {
  std::vector<Variable> binWidths;
  for (const auto &dim : dims) {
    const auto &coord = c[dim];
    if (coord.unit() == sc_units::dimensionless)
      throw std::runtime_error("Dimensionless axis cannot be used for "
                               "conversion from or to density");
    binWidths.emplace_back(coord.slice({dim, 1, coord.dims()[dim]}) -
                           coord.slice({dim, 0, coord.dims()[dim] - 1}));
  }
  return binWidths;
}

void toDensity(DataArray &data, const std::vector<Variable> &binWidths) {
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

Dataset toDensity(const Dataset &d, const Dim dim) {
  return toDensity(d, std::vector<Dim>{dim});
}

Dataset toDensity(const Dataset &d, const std::vector<Dim> &dims) {
  const auto binWidths = getBinWidths(d.coords(), dims);
  for (auto &&data : d)
    toDensity(data, binWidths);
  return d;
}

DataArray toDensity(const DataArray &a, const Dim dim) {
  return toDensity(a, std::vector<Dim>{dim});
}

DataArray toDensity(const DataArray &a, const std::vector<Dim> &dims) {
  const auto binWidths = getBinWidths(a.coords(), dims);
  auto out = copy(a);
  toDensity(out, binWidths);
  return out;
}

void fromDensity(DataArray &data, const std::vector<Variable> &binWidths) {
  if (data.unit().isCounts()) {
    // Do nothing, but do not fail either.
  } else if (data.unit().isCountDensity()) {
    for (const auto &binWidth : binWidths)
      data *= binWidth;
  }
}

Dataset fromDensity(const Dataset &d, const Dim dim) {
  return fromDensity(d, std::vector<Dim>{dim});
}

Dataset fromDensity(const Dataset &d, const std::vector<Dim> &dims) {
  const auto binWidths = getBinWidths(d.coords(), dims);
  for (auto &&data : d)
    fromDensity(data, binWidths);
  return d;
}

DataArray fromDensity(const DataArray &a, const Dim dim) {
  return fromDensity(a, std::vector<Dim>{dim});
}

DataArray fromDensity(const DataArray &a, const std::vector<Dim> &dims) {
  const auto binWidths = getBinWidths(a.coords(), dims);
  auto out = copy(a);
  fromDensity(out, binWidths);
  return out;
}

} // namespace counts
} // namespace scipp::dataset
