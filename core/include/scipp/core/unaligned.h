// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_UNALIGNED_H
#define SCIPP_CORE_UNALIGNED_H

#include <set>

#include "scipp/core/dataset.h"

namespace scipp::core::unaligned {

Dim unaligned_dim(const VariableConstView &unaligned);

template <class CoordMap = std::vector<std::pair<Dim, Variable>>>
DataArray realign(DataArray unaligned, CoordMap coords) {
  std::set<Dim> binnedDims;
  std::set<Dim> unalignedDims;
  for (const auto &item : coords)
    binnedDims.insert(item.first);
  for (const auto &[dim, coord] : unaligned.coords())
    if (binnedDims.count(dim))
      unalignedDims.insert(unaligned_dim(coord));
  if (unalignedDims.size() < 1)
    throw except::UnalignedError("realign requires at least one unaligned "
                                 "dimension.");
  if (unalignedDims.size() > 1)
    throw except::UnalignedError(
        "realign with more than one unaligned dimension not supported yet.");

  // TODO Some things here can be simplified and optimized by adding an
  // `extract` method to MutableView.
  std::vector<std::pair<Dim, Variable>> alignedCoords;
  const auto dims = unaligned.dims();
  for (const auto &dim : dims.labels()) {
    if (unalignedDims.count(dim)) {
      alignedCoords.insert(alignedCoords.end(), coords.begin(), coords.end());
    } else {
      alignedCoords.emplace_back(dim, Variable(unaligned.coords()[dim]));
      unaligned.coords().erase(dim);
    }
  }
  auto name = unaligned.name();
  return DataArray(Variable{}, std::move(alignedCoords), {}, {},
                   std::move(name), std::move(unaligned));
}

} // namespace scipp::core::unaligned

#endif // SCIPP_CORE_EVENT_H
